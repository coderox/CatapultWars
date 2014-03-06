#include "pch.h"
#include "CatapultGame.h"
#include "DDSTextureLoader.h"

namespace CatapultWars{

	CatapultGame::CatapultGame() :
		m_minWind(0),
		m_maxWind(2)
	{
		//EnabledGestures = GestureType.FreeDrag | GestureType.DragComplete | GestureType.Tap;
	}

	void CatapultGame::CreateDeviceResources()
	{
		Direct3DBase::CreateDeviceResources();
	}

	void CatapultGame::CreateWindowSizeDependentResources()
	{
		Direct3DBase::CreateWindowSizeDependentResources();

		auto device = m_d3dDevice.Get();
		auto context = m_d3dContext.Get();

		m_spriteBatch.reset(new SpriteBatch(context));
		m_spriteBatch->SetRotation(DXGI_MODE_ROTATION_ROTATE90);

		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\sky.dds", nullptr, m_skyTexture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\gameplay_screen.dds", nullptr, m_foregroundTexture.ReleaseAndGetAddressOf())
			);
		ComPtr<ID3D11Resource> cloud1TextureRes;
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\cloud1.dds", cloud1TextureRes.ReleaseAndGetAddressOf(), m_cloud1Texture.ReleaseAndGetAddressOf())
			);
		ComPtr<ID3D11Resource> cloud2TextureRes;
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\cloud2.dds", cloud2TextureRes.ReleaseAndGetAddressOf(), m_cloud2Texture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\mountain.dds", nullptr, m_mountainTexture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\defeat.dds", nullptr, m_defeatTexture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\Backgrounds\\victory.dds", nullptr, m_victoryTexture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\HUD\\hudBackground.dds", nullptr, m_hudBackgroundTexture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\HUD\\windArrow.dds", nullptr, m_windArrowTexture.ReleaseAndGetAddressOf())
			);
		DX::ThrowIfFailed(
			CreateDDSTextureFromFile(device, L"Assets\\Textures\\HUD\\ammoType.dds", nullptr, m_ammoTypeTexture.ReleaseAndGetAddressOf())
			);

		// Load font
		m_hudFont.reset(new SpriteFont(device, L"Assets\\Fonts\\TestHUDFont.spritefont"));

		// Define initial cloud position

		DX::GetTextureSize(cloud1TextureRes.Get(), &m_cloud1TextureWidth, &m_cloud1TextureHeight);
		DX::GetTextureSize(cloud2TextureRes.Get(), &m_cloud2TextureWidth, &m_cloud2TextureHeight);

		m_cloud1Position = Vector2(224 - m_cloud1TextureWidth, 32);
		m_cloud2Position = Vector2(64, 90);

		// Define initial HUD positions
		m_playerHUDPosition = Vector2(7, 7);
		m_computerHUDPosition = Vector2(613, 7);
		m_windArrowPosition = Vector2(345, 46);

		// Initialize human & AI players
		m_player = ref new Human();
		m_player->Initialize(device, m_spriteBatch);
		m_player->Name = L"Player";

		m_computer = ref new AI();
		m_computer->Initialize(device, m_spriteBatch);
		m_computer->Name = L"Phone";

		m_player->Enemy = m_computer;
		m_computer->Enemy = m_player;

		m_viewportWidth = 800;
		m_viewportHeight = 480;

		Start();
	}

	void CatapultGame::Start()
	{
		m_wind = Vector2(0, 0);
		m_isHumanTurn = false;
		m_changeTurn = true;
		m_computer->Catapult->CurrentState = CatapultState::Reset;
	}

	void CatapultGame::Update(float timeTotal, float timeDelta)
	{
		float elapsed = timeDelta;

		// Check it one of the players reached 5 and stop the game
		if ((m_player->Catapult->GameOver || m_computer->Catapult->GameOver) &&
			(m_gameOver == false))
		{
			m_gameOver = true;

			if (m_player->Score > m_computer->Score)
			{
				//AudioManager.PlaySound("gameOver_Win");
			}
			else
			{
				//AudioManager.PlaySound("gameOver_Lose");
			}

			return;
		}

		// If Reset flag raised and both catapults are not animating - 
		// active catapult finished the cycle, new turn!
		if ((m_player->Catapult->CurrentState == CatapultState::Reset ||
			m_computer->Catapult->CurrentState == CatapultState::Reset) &&
			!(m_player->Catapult->AnimationRunning ||
			m_computer->Catapult->AnimationRunning))
		{
			m_changeTurn = true;

			if (m_player->IsActive == true) //Last turn was a human turn?
			{
				m_player->IsActive = false;
				m_computer->IsActive = true;
				m_isHumanTurn = false;
				m_player->Catapult->CurrentState = CatapultState::Idle;
				m_computer->Catapult->CurrentState = CatapultState::Aiming;
			}
			else //It was an AI turn
			{
				m_player->IsActive = true;
				m_computer->IsActive = false;
				m_isHumanTurn = true;
				m_computer->Catapult->CurrentState = CatapultState::Idle;
				m_player->Catapult->CurrentState = CatapultState::Idle;
			}
		}

		if (m_changeTurn)
		{
			// Update wind
			m_wind = Vector2(rand() % 4 - 1, rand() % (m_maxWind + 1) + m_minWind);

			// Set new wind value to the players and 
			m_player->Catapult->Wind = m_wind.x > 0 ? m_wind.y : -m_wind.y;
			m_computer->Catapult->Wind = m_wind.x > 0 ? m_wind.y : -m_wind.y;
			m_changeTurn = false;
		}

		// Update the players
		m_player->Update(timeTotal, timeDelta);
		m_computer->Update(timeTotal, timeDelta);

		// Updates the clouds position
		UpdateClouds(elapsed);
	}

	void CatapultGame::UpdateClouds(float elapsedTime)
	{
		// Move the clouds according to the wind
		int windDirection = m_wind.x > 0 ? 1 : -1;

		m_cloud1Position += Vector2(24.0f, 0.0f) * elapsedTime * windDirection * m_wind.y;
		if (m_cloud1Position.x > m_viewportWidth)
			m_cloud1Position.x = -(int)m_cloud1TextureWidth * 2.0f;
		else if (m_cloud1Position.x < -(int)m_cloud1TextureWidth * 2.0f)
			m_cloud1Position.x = m_viewportWidth;

		m_cloud2Position += Vector2(16.0f, 0.0f) * elapsedTime * windDirection * m_wind.y;
		if (m_cloud2Position.x > m_viewportWidth)
			m_cloud2Position.x = -(int)m_cloud2TextureWidth * 2.0f;
		else if (m_cloud2Position.x < -(int)m_cloud2TextureWidth * 2.0f)
			m_cloud2Position.x = m_viewportWidth;
	}

	void CatapultGame::Render()
	{
		const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
		const float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_d3dContext->ClearRenderTargetView(
			m_renderTargetView.Get(),
			white
			);

		m_d3dContext->ClearDepthStencilView(
			m_depthStencilView.Get(),
			D3D11_CLEAR_DEPTH,
			1.0f,
			0
			);
		m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

		m_spriteBatch->Begin();

		DrawBackground();
		DrawComputer();
		DrawPlayer();
		DrawHud();

		m_spriteBatch->End();
	}

	void CatapultGame::DrawHud()
	{
		if (m_gameOver)
		{
			ComPtr<ID3D11ShaderResourceView> texture;
			if (m_player->Score > m_computer->Score)
			{
				texture = m_victoryTexture;
			}
			else
			{
				texture = m_defeatTexture;
			}

			m_spriteBatch->Draw(texture.Get(), Vector2(m_viewportWidth / 2, m_viewportHeight / 2), Colors::White);

			/*ScreenManager.SpriteBatch.Draw(
				texture,
				new Vector2(ScreenManager.Game.GraphicsDevice.Viewport.Width / 2 - texture.Width / 2,
				ScreenManager.Game.GraphicsDevice.Viewport.Height / 2 - texture.Height / 2),
				Color.White);*/
		}
		else
		{
			// Draw Player Hud
			m_spriteBatch->Draw(m_hudBackgroundTexture.Get(), m_playerHUDPosition, Colors::White);
			m_spriteBatch->Draw(m_ammoTypeTexture.Get(), m_playerHUDPosition + Vector2(33, 35), Colors::White);
			DrawString(m_hudFont, m_player->Score.ToString(), m_playerHUDPosition + Vector2(123, 35), Colors::White);
			DrawString(m_hudFont, m_player->Name, m_playerHUDPosition + Vector2(40, 1), Colors::Blue);

			// Draw Computer Hud
			m_spriteBatch->Draw(m_hudBackgroundTexture.Get(), m_computerHUDPosition, Colors::White);
			m_spriteBatch->Draw(m_ammoTypeTexture.Get(), m_computerHUDPosition + Vector2(33, 35), Colors::White);
			DrawString(m_hudFont, m_computer->Score.ToString(), m_computerHUDPosition + Vector2(123, 35), Colors::White);
			DrawString(m_hudFont, m_computer->Name, m_computerHUDPosition + Vector2(40, 1), Colors::Red);

			// Draw Wind direction
			Platform::String^ text = "WIND";
			Vector2 size = m_hudFont->MeasureString(text->Data());
			Vector2 windarrowScale = Vector2(m_wind.y / 10, 1);
			m_spriteBatch->Draw(m_windArrowTexture.Get(),
				m_windArrowPosition, nullptr, Colors::White, 0, Vector2(0,0),
				windarrowScale, m_wind.x > 0 ? SpriteEffects::SpriteEffects_None : SpriteEffects::SpriteEffects_FlipHorizontally, 0);

			DrawString(m_hudFont, text, m_windArrowPosition - Vector2(0, size.y), Colors::Black);
			if (m_wind.y == 0)
			{
				text = "NONE";
				DrawString(m_hudFont, text, m_windArrowPosition, Colors::Black);
			}

			if (m_isHumanTurn)
			{
				// Prepare human prompt message
				text = !m_isDragging ?
					"Drag Anywhere to Fire" : "Release to Fire!";
				size = m_hudFont->MeasureString(text->Data());
			}
			else
			{
				// Prepare AI message
				text = "I'll get you yet!";
				size = m_hudFont->MeasureString(text->Data());
			}

			DrawString(m_hudFont,text,
				Vector2(
				m_viewportWidth / 2 - size.x / 2,
				m_viewportHeight - size.y),
				Colors::Green);
		}
	}

	void CatapultGame::DrawComputer()
	{
		if (!m_gameOver)
			m_computer->Draw();
	}

	void CatapultGame::DrawPlayer()
	{
		if (!m_gameOver)
			m_player->Draw();
	}

	void CatapultGame::DrawString(std::shared_ptr<SpriteFont> font, Platform::String^ text, Vector2 position, FXMVECTOR color)
	{
		font->DrawString(m_spriteBatch.get(), text->Data(), position, Colors::Black, 0, Vector2(0, 0), Vector2(1, 1), SpriteEffects::SpriteEffects_None, 0);
		font->DrawString(m_spriteBatch.get(), text->Data(), position + Vector2(1, 1), color, 0, Vector2(0, 0), Vector2(1, 1), SpriteEffects::SpriteEffects_None, 0);
	}

	void CatapultGame::DrawString(std::shared_ptr<SpriteFont> font, Platform::String^ text, Vector2 position, FXMVECTOR color, float fontScale)
	{
		font->DrawString(m_spriteBatch.get(), text->Data(), position, Colors::Black, 0, Vector2(0, 0), Vector2(1, 1), SpriteEffects::SpriteEffects_None, 0);
		font->DrawString(m_spriteBatch.get(), text->Data(), position + Vector2(1, 1), color, 0, Vector2(0, 0), Vector2(fontScale, fontScale), SpriteEffects::SpriteEffects_None, 0);
	}

	void CatapultGame::DrawBackground()
	{
		m_spriteBatch->Draw(m_skyTexture.Get(), Vector2(0, 0), Colors::White);

		m_spriteBatch->Draw(m_cloud1Texture.Get(), m_cloud1Position, Colors::White);

		// Draw the Mountain
		m_spriteBatch->Draw(m_mountainTexture.Get(), Vector2(0, 0), Colors::White);

		// Draw Cloud #2
		m_spriteBatch->Draw(m_cloud2Texture.Get(), m_cloud2Position, Colors::White);

		// Draw the Castle, trees, and foreground 
		m_spriteBatch->Draw(m_foregroundTexture.Get(), Vector2(0, 0), Colors::White);
	}
}