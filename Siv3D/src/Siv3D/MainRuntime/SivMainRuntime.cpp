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

# include <Siv3D/MainRuntime.hpp>
# include <Siv3D/EngineOptions.hpp>
# include <Siv3D/Error.hpp>
# include <Siv3D/Common/Siv3DEngine.hpp>
# include <Siv3D/System/ISystem.hpp>

# if SIV3D_PLATFORM(WINDOWS)
#	include <Siv3D/Window/IWindow.hpp>
#	include <Siv3D/System/CSystem.hpp>
# endif

namespace s3d
{
	namespace detail
	{
		class MainRuntimeDetail
		{
		public:

			MainRuntimeDetail()
			{
				if (Siv3DEngine::isActive())
				{
					throw EngineError{ U"Siv3D engine has already been initialized" };
				}

				if (g_engineOptions.renderer != EngineOption::Renderer::Headless)
				{
					throw EngineError{ U"s3d::MainRuntime requires SIV3D_SET(EngineOption::Renderer::Headless)" };
				}

				m_engine = std::make_unique<Siv3DEngine>();

				SIV3D_ENGINE(System)->init();

			# if SIV3D_PLATFORM(WINDOWS)

				SIV3D_ENGINE(Window)->init();
				static_cast<CSystem*>(SIV3D_ENGINE(System))->init2();

			# endif
			}

			~MainRuntimeDetail() = default;

		private:

			std::unique_ptr<Siv3DEngine> m_engine;
		};
	}

	MainRuntime::MainRuntime()
		: pImpl{ std::make_unique<detail::MainRuntimeDetail>() } {}

	MainRuntime::~MainRuntime() = default;
}
