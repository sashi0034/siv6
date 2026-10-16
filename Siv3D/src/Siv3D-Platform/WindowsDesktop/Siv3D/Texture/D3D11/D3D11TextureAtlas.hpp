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

# pragma once
# include <memory>
# include <Siv3D/Array.hpp>
# include <Siv3D/HashTable.hpp>
# include <Siv3D/Texture/ITexture.hpp>
# include <Siv3D/AssetHandleManager/AssetHandleManager.hpp>
# include "D3D11Texture.hpp"
# include "D3D11TextureAtlasAllocator.hpp"

namespace s3d
{
	class D3D11TextureAtlas
	{
	private:

		static constexpr Size AtlasSize{ 4096, 4096 };

		static constexpr int32 Padding = 1;

		struct CacheKey
		{
			Texture::IDType textureID;

			Rect sourceRegion;

			[[nodiscard]]
			bool operator ==(const CacheKey&) const noexcept = default;
		};

		struct CacheKeyHash
		{
			[[nodiscard]]
			size_t operator ()(const CacheKey& key) const noexcept;
		};

		struct Page
		{
			TextureFormat format;

			TextureDesc desc = TextureDesc::Unmipped;

			std::shared_ptr<Texture::AssetIDWrapperType> textureHandle;

			D3D11TextureAtlasAllocator allocator{ AtlasSize };
		};

		struct PendingCopy
		{
			Texture source;

			Texture::IDType destinationID;

			Rect sourceRegion;

			Point destination;
		};

		ID3D11Device* m_device = nullptr;

		ID3D11DeviceContext* m_context = nullptr;

		AssetHandleManager<Texture::IDType, D3D11Texture>& m_textures;

		HashTable<CacheKey, detail::TextureAtlasRegionData, CacheKeyHash> m_cache;

		Array<Page> m_pages;

		Array<PendingCopy> m_pendingCopies;

		[[nodiscard]]
		Page* createPage(const TextureFormat& format, TextureDesc desc);

	public:

		D3D11TextureAtlas(ID3D11Device* device, ID3D11DeviceContext* context,
			AssetHandleManager<Texture::IDType, D3D11Texture>& textures);

		[[nodiscard]]
		detail::TextureAtlasRegionData getRegion(const Texture& source, const Rect& sourceRegion);

		void clear();

		void flush();
	};
}
