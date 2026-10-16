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
# include <Siv3D/Optional.hpp>
# include <Siv3D/Rect.hpp>

namespace s3d
{
	class D3D11TextureAtlasAllocator
	{
	private:

		struct FreeSize
		{
			int32 width = 0;

			int32 height = 0;

			int32 shortSide = 0;

			[[nodiscard]]
			bool mightContain(const Size& size) const noexcept
			{
				return ((size.x <= width)
					&& (size.y <= height)
					&& (Min(size.x, size.y) <= shortSide));
			}

			[[nodiscard]]
			static FreeSize FromRect(const Rect& rect) noexcept
			{
				return{ rect.w, rect.h, Min(rect.w, rect.h) };
			}

			[[nodiscard]]
			static FreeSize Union(const FreeSize& a, const FreeSize& b) noexcept
			{
				return{ Max(a.width, b.width), Max(a.height, b.height), Max(a.shortSide, b.shortSide) };
			}
		};

		struct Node
		{
			Rect region;

			FreeSize maxFree;

			bool used = false;

			std::unique_ptr<Node> child0;

			std::unique_ptr<Node> child1;

			explicit Node(const Rect& _region)
				: region{ _region }
				, maxFree{ FreeSize::FromRect(_region) } {}

			[[nodiscard]]
			bool isLeaf() const noexcept
			{
				return (not child0);
			}

			void updateMaxFree() noexcept
			{
				if (child0)
				{
					maxFree = FreeSize::Union(child0->maxFree, child1->maxFree);
				}
				else if (used)
				{
					maxFree = {};
				}
				else
				{
					maxFree = FreeSize::FromRect(region);
				}
			}
		};

		Node m_root;

		[[nodiscard]]
		static double Squareness(const int32 width, const int32 height) noexcept
		{
			if ((width <= 0) || (height <= 0))
			{
				return 0.0;
			}

			return (static_cast<double>(Min(width, height)) / Max(width, height));
		}

		[[nodiscard]]
		static Optional<Rect> Allocate(Node& node, const Size& size)
		{
			if (node.used || (not node.maxFree.mightContain(size)))
			{
				return none;
			}

			if (not node.isLeaf())
			{
				if (auto result = Allocate(*node.child0, size))
				{
					node.updateMaxFree();
					return result;
				}

				auto result = Allocate(*node.child1, size);
				node.updateMaxFree();
				return result;
			}

			if ((node.region.w == size.x) && (node.region.h == size.y))
			{
				node.used = true;
				node.updateMaxFree();
				return node.region;
			}

			const int32 remainingWidth = (node.region.w - size.x);
			const int32 remainingHeight = (node.region.h - size.y);
			const bool splitVertically =
				(Squareness(remainingWidth, node.region.h) >= Squareness(node.region.w, remainingHeight));

			if (splitVertically)
			{
				node.child0 = std::make_unique<Node>(Rect{ node.region.pos, size.x, node.region.h });
				node.child1 = std::make_unique<Node>(Rect{
					(node.region.x + size.x), node.region.y, remainingWidth, node.region.h });
			}
			else
			{
				node.child0 = std::make_unique<Node>(Rect{ node.region.pos, node.region.w, size.y });
				node.child1 = std::make_unique<Node>(Rect{
					node.region.x, (node.region.y + size.y), node.region.w, remainingHeight });
			}

			auto result = Allocate(*node.child0, size);
			node.updateMaxFree();
			return result;
		}

	public:

		explicit D3D11TextureAtlasAllocator(const Size& size)
			: m_root{ Rect{ size } } {}

		[[nodiscard]]
		Optional<Rect> allocate(const Size& size)
		{
			if ((size.x <= 0) || (size.y <= 0))
			{
				return none;
			}

			return Allocate(m_root, size);
		}
	};
}
