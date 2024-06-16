#include <pandora.hpp>

#include "game.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Game::Game()
{

}

Game::~Game()
{

}

void Game::EntryPoint()
{
    Pandora::Initialize();
    Pandora::Shutdown();
}

} // namespace WingsOfSteel::TheBrightestStar