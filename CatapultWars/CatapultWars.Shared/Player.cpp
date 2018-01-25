#include "pch.h"

using namespace std;
using namespace DirectX;
using namespace CatapultWars;

Player::Player()
	: MinShotStrength(150)
	, MaxShotStrength(400)
{

}

void Player::Initialize(ID3D11Device* device, shared_ptr<SpriteBatch>& spriteBatch, shared_ptr<AudioManager>& audioManager)
{
	m_score = 0;
}

void Player::Update(double elapsedSeconds)
{
	Catapult->Update(elapsedSeconds);
}

void Player::Draw()
{
	Catapult->Draw();
}