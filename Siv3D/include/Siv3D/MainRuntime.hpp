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
# include "Common.hpp"
# include "Uncopyable.hpp"
# include <memory>

namespace s3d
{
	namespace detail
	{
		class MainRuntimeDetail;
	}

	/// @brief Siv3D の Main() に依存せず、エンジンの初期化と終了処理を RAII で管理するクラス
	/// @remark 現在は EngineOption::Renderer::Headless のみ対応しています
	class MainRuntime : Uncopyable
	{
	public:

		SIV3D_NODISCARD_CXX20
		MainRuntime();

		~MainRuntime();

	private:

		std::unique_ptr<detail::MainRuntimeDetail> pImpl;
	};
}
