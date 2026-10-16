//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2025 Ryo Suzuki
//	Copyright (c) 2016-2025 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# include "D3D11TextureAtlas.hpp"
# include <array>
# include <Siv3D/AssetMonitor/IAssetMonitor.hpp>
# include <Siv3D/Common/Siv3DEngine.hpp>
# include <Siv3D/Hash.hpp>
# include <Siv3D/SamplerState.hpp>
# include <Siv3D/Texture/TextureCommon.hpp>

namespace s3d
{
	size_t D3D11TextureAtlas::CacheKeyHash::operator ()(const CacheKey& key) const noexcept
	{
		size_t result = 0;
		Hash::Combine(result, key.textureID);
		Hash::Combine(result, key.sourceRegion);
		return result;
	}

	D3D11TextureAtlas::D3D11TextureAtlas(ID3D11Device* const device, ID3D11DeviceContext* const context,
		AssetHandleManager<Texture::IDType, D3D11Texture>& textures)
		: m_device{ device }
		, m_context{ context }
		, m_textures{ textures } {}

	D3D11TextureAtlas::Page* D3D11TextureAtlas::createPage(const TextureFormat& format, const TextureDesc desc)
	{
		auto texture = std::make_unique<D3D11Texture>(
			D3D11Texture::Render{}, m_device, AtlasSize, format, desc, HasDepth::No);

		if (not texture->isInitialized())
		{
			return nullptr;
		}

		texture->clearRT(m_context, ColorF{ 0.0, 0.0, 0.0, 0.0 });

		const String info = U"(type: TextureAtlas, size: {0}x{1}, format: {2})"_fmt(
			AtlasSize.x, AtlasSize.y, format.name());
		const Texture::IDType textureID = m_textures.add(std::move(texture), info);
		if (textureID.isNull())
		{
			return nullptr;
		}
		SIV3D_ENGINE(AssetMonitor)->created();

		Page page;
		page.format = format;
		page.desc = desc;
		page.textureHandle = std::make_shared<Texture::AssetIDWrapperType>(textureID);
		m_pages.push_back(std::move(page));
		return &m_pages.back();
	}

	detail::TextureAtlasRegionData D3D11TextureAtlas::getRegion(const Texture& source, const Rect& sourceRegion)
	{
		if (not source
			|| (sourceRegion.x < 0) || (sourceRegion.y < 0)
			|| (sourceRegion.w <= 0) || (sourceRegion.h <= 0))
		{
			return{};
		}

		const CacheKey cacheKey{ source.id(), sourceRegion };
		if (const auto it = m_cache.find(cacheKey); it != m_cache.end())
		{
			return it->second;
		}

		const Size sourceSize = source.size();
		if ((sourceRegion.w > sourceSize.x) || (sourceRegion.h > sourceSize.y)
			|| (sourceRegion.w > (AtlasSize.x - Padding * 2))
			|| (sourceRegion.h > (AtlasSize.y - Padding * 2))
			|| (sourceRegion.x > (sourceSize.x - sourceRegion.w))
			|| (sourceRegion.y > (sourceSize.y - sourceRegion.h)))
		{
			return{};
		}

		D3D11Texture* const sourceTexture = m_textures[source.id()];
		if ((not sourceTexture) || (not sourceTexture->isDefault()))
		{
			return{};
		}

		const TextureFormat format = sourceTexture->getDesc().format;
		if ((format != TextureFormat::R8G8B8A8_Unorm)
			&& (format != TextureFormat::R8G8B8A8_Unorm_SRGB))
		{
			return{};
		}

		const TextureDesc sourceDesc = sourceTexture->getDesc().desc;
		const TextureDesc atlasDesc = detail::IsSDF(sourceDesc)
			? TextureDesc::SDF
			: (format.isSRGB() ? TextureDesc::UnmippedSRGB : TextureDesc::Unmipped);
		const Size allocationSize{
			(sourceRegion.w + Padding * 2), (sourceRegion.h + Padding * 2) };

		Page* targetPage = nullptr;
		Optional<Rect> allocation;
		for (auto& page : m_pages)
		{
			if ((page.format == format) && (page.desc == atlasDesc))
			{
				if (auto candidate = page.allocator.allocate(allocationSize))
				{
					targetPage = &page;
					allocation = candidate;
					break;
				}
			}
		}

		if (not targetPage)
		{
			targetPage = createPage(format, atlasDesc);
			if (not targetPage)
			{
				return{};
			}

			allocation = targetPage->allocator.allocate(allocationSize);
			if (not allocation)
			{
				return{};
			}
		}

		const Rect atlasRegion{
			allocation->x + Padding,
			allocation->y + Padding,
			sourceRegion.w,
			sourceRegion.h
		};
		const detail::TextureAtlasRegionData result{ targetPage->textureHandle, atlasRegion };
		m_cache.emplace(cacheKey, result);
		m_pendingCopies.push_back(PendingCopy{
			source, targetPage->textureHandle->id(), sourceRegion, atlasRegion.pos });
		return result;
	}

	void D3D11TextureAtlas::clear()
	{
		m_pendingCopies.clear();
		m_cache.clear();
		m_pages.clear();
	}

	void D3D11TextureAtlas::flush()
	{
		if (m_pendingCopies.isEmpty())
		{
			return;
		}

		std::array<ID3D11ShaderResourceView*, SamplerState::MaxSamplerCount> nullSRVs{};
		m_context->VSSetShaderResources(0, static_cast<UINT>(nullSRVs.size()), nullSRVs.data());
		m_context->PSSetShaderResources(0, static_cast<UINT>(nullSRVs.size()), nullSRVs.data());

		for (const auto& copy : m_pendingCopies)
		{
			ID3D11Texture2D* const destinationTexture = m_textures[copy.destinationID]->getTexture();
			ID3D11Texture2D* const sourceTexture = m_textures[copy.source.id()]->getTexture();

			const auto copyRegion = [this, destinationTexture, sourceTexture](
				const Rect& sourceRegion, const Point& destination)
			{
				const D3D11_BOX sourceBox{
					static_cast<UINT>(sourceRegion.x),
					static_cast<UINT>(sourceRegion.y),
					0,
					static_cast<UINT>(sourceRegion.x + sourceRegion.w),
					static_cast<UINT>(sourceRegion.y + sourceRegion.h),
					1
				};

				m_context->CopySubresourceRegion(
					destinationTexture, 0,
					static_cast<UINT>(destination.x), static_cast<UINT>(destination.y), 0,
					sourceTexture, 0, &sourceBox);
			};

			const int32 left = copy.sourceRegion.x;
			const int32 top = copy.sourceRegion.y;
			const int32 right = (copy.sourceRegion.x + copy.sourceRegion.w - 1);
			const int32 bottom = (copy.sourceRegion.y + copy.sourceRegion.h - 1);
			const int32 width = copy.sourceRegion.w;
			const int32 height = copy.sourceRegion.h;

			copyRegion(copy.sourceRegion, copy.destination);

			copyRegion(Rect{ left, top, width, 1 }, copy.destination.movedBy(0, -Padding));
			copyRegion(Rect{ left, bottom, width, 1 }, copy.destination.movedBy(0, height));
			copyRegion(Rect{ left, top, 1, height }, copy.destination.movedBy(-Padding, 0));
			copyRegion(Rect{ right, top, 1, height }, copy.destination.movedBy(width, 0));

			copyRegion(Rect{ left, top, 1 }, copy.destination.movedBy(-Padding, -Padding));
			copyRegion(Rect{ right, top, 1 }, copy.destination.movedBy(width, -Padding));
			copyRegion(Rect{ left, bottom, 1 }, copy.destination.movedBy(-Padding, height));
			copyRegion(Rect{ right, bottom, 1 }, copy.destination.movedBy(width, height));
		}

		m_pendingCopies.clear();
	}
}
