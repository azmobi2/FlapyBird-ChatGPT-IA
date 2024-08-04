#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

const int largura_tela = 800;
const int altura_tela = 700;
const int largura_pássaro = 50;
const int altura_pássaro = 50;
const int largura_bolha = largura_pássaro * 2;
const int altura_bolha = altura_pássaro * 2;
const int largura_cano = 50;
const int intervalo_canos_inicial = 200; // Diminuído o espaço inicial entre canos
const int velocidade_pássaro = 10;
const int gravidade = 1;
const int vidas_iniciais = 30;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* imagem_fundo = nullptr;
SDL_Texture* imagem_cano = nullptr;
SDL_Texture* imagem_pássaro = nullptr;
SDL_Texture* imagem_bolha = nullptr;
Mix_Chunk* som_pulo = nullptr;
Mix_Chunk* som_colisao = nullptr;
TTF_Font* font = nullptr;
TTF_Font* font_game_over = nullptr;
TTF_Font* font_iniciar = nullptr;

struct Cano {
    int x, y;
    bool subir;
};

struct Bolha {
    int x, y;
};

vector<Cano> canos;
vector<Bolha> bolhas;

void carregarRecursos() {
    imagem_fundo = IMG_LoadTexture(renderer, "fundo.png");
    imagem_cano = IMG_LoadTexture(renderer, "cano.png");
    imagem_pássaro = IMG_LoadTexture(renderer, "passaro.png");
    imagem_bolha = IMG_LoadTexture(renderer, "bolha.png");
    som_pulo = Mix_LoadWAV("pulo.wav");
    som_colisao = Mix_LoadWAV("colisao.wav");
    font = TTF_OpenFont("arial.ttf", 24);
    font_game_over = TTF_OpenFont("arial.ttf", 14);
    font_iniciar = TTF_OpenFont("arial.ttf", 12);
}

void atualizarTela(int pontuacao, int vidas, int x_pássaro, int y_pássaro, int intervalo_canos, bool game_over) {
    SDL_RenderClear(renderer);

    // Atualiza a imagem de fundo de acordo com a pontuação
    if (pontuacao >= 90) {
        SDL_DestroyTexture(imagem_fundo);
        imagem_fundo = IMG_LoadTexture(renderer, "fundo_final.png");
    } else if (pontuacao >= 61) {
        SDL_DestroyTexture(imagem_fundo);
        imagem_fundo = IMG_LoadTexture(renderer, "caminho.png");
    } else if (pontuacao >= 36) {
        SDL_DestroyTexture(imagem_fundo);
        imagem_fundo = IMG_LoadTexture(renderer, "noite.png");
    } else {
        SDL_DestroyTexture(imagem_fundo);
        imagem_fundo = IMG_LoadTexture(renderer, "fundo.png");
    }

    SDL_Rect destino = {0, 0, largura_tela, altura_tela};
    SDL_RenderCopy(renderer, imagem_fundo, nullptr, &destino);

    for (const auto& cano : canos) {
        SDL_Rect rect_cano_superior = {cano.x, 0, largura_cano, cano.y};
        SDL_Rect rect_cano_inferior = {cano.x, cano.y + intervalo_canos, largura_cano, altura_tela - cano.y - intervalo_canos};
        SDL_RenderCopy(renderer, imagem_cano, nullptr, &rect_cano_superior);
        SDL_RenderCopy(renderer, imagem_cano, nullptr, &rect_cano_inferior);
    }

    for (const auto& bolha : bolhas) {
        SDL_Rect rect_bolha = {bolha.x, bolha.y, largura_bolha, altura_bolha};
        SDL_RenderCopy(renderer, imagem_bolha, nullptr, &rect_bolha);
    }

    SDL_Rect rect_pássaro = {x_pássaro, y_pássaro, largura_pássaro, altura_pássaro};
    SDL_RenderCopy(renderer, imagem_pássaro, nullptr, &rect_pássaro);

    stringstream texto;
    texto << "Pontuacao: " << pontuacao << " Vidas: " << vidas;
    SDL_Color cor = {255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, texto.str().c_str(), cor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    int texto_largura, texto_altura;
    SDL_QueryTexture(texture, nullptr, nullptr, &texto_largura, &texto_altura);
    SDL_Rect dest = {10, 10, texto_largura, texto_altura};
    SDL_RenderCopy(renderer, texture, nullptr, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    if (game_over) {
        SDL_Color cor_game_over = {255, 0, 0};
        surface = TTF_RenderText_Solid(font_game_over, "Game Over", cor_game_over);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_QueryTexture(texture, nullptr, nullptr, &texto_largura, &texto_altura);
        dest = {largura_tela / 2 - texto_largura / 2, altura_tela / 2 - texto_altura / 2 - 20, texto_largura, texto_altura};
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        SDL_Color cor_iniciar = {0, 255, 0};
        surface = TTF_RenderText_Solid(font_iniciar, "Iniciar", cor_iniciar);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_QueryTexture(texture, nullptr, nullptr, &texto_largura, &texto_altura);
        dest = {largura_tela / 2 - texto_largura / 2, altura_tela / 2 - texto_altura / 2 + 20, texto_largura, texto_altura};
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

void jogo() {
    bool rodando = true;
    bool pulando = false;
    bool game_over = false;
    int pontuacao = 0;
    int vidas = vidas_iniciais;
    int x_pássaro = largura_tela / 4;
    int y_pássaro = altura_tela / 2;
    int velocidade_cano = 2; // Velocidade horizontal baixa
    int velocidade_y_pássaro = 0;
    int intervalo_canos = intervalo_canos_inicial;

    srand(time(0));

    int y_inicial_cano = rand() % (altura_tela - intervalo_canos - 150);
    if (pontuacao <= 15) {
        y_inicial_cano += 50; // Abaixar a abertura do cano
    }
    canos.push_back({largura_tela, y_inicial_cano, true});

    SDL_Event evento;

    while (rodando) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                rodando = false;
            } else if (evento.type == SDL_FINGERDOWN || evento.type == SDL_MOUSEBUTTONDOWN) {
                if (game_over) {
                    game_over = false;
                    pontuacao = 0;
                    vidas = vidas_iniciais;
                    x_pássaro = largura_tela / 4;
                    y_pássaro = altura_tela / 2;
                    velocidade_y_pássaro = 0;
                    intervalo_canos = intervalo_canos_inicial;
                    canos.clear();
                    y_inicial_cano = rand() % (altura_tela - intervalo_canos - 150);
                    if (pontuacao <= 15) {
                        y_inicial_cano += 50; // Abaixar a abertura do cano
                    }
                    canos.push_back({largura_tela, y_inicial_cano, true});
                } else {
                    pulando = true;
                }
            }
        }

        if (!game_over) {
            if (pulando) {
                velocidade_y_pássaro = -10;
                Mix_PlayChannel(-1, som_pulo, 0);
                pulando = false;
            } else {
                velocidade_y_pássaro += gravidade;
            }
            y_pássaro += velocidade_y_pássaro;

            if (y_pássaro + altura_pássaro > altura_tela) {
                y_pássaro = altura_tela - altura_pássaro;
                velocidade_y_pássaro = 0;
            } else if (y_pássaro < 0) {
                y_pássaro = 0;
                velocidade_y_pássaro = 0;
            }

            for (auto& cano : canos) {
                cano.x -= velocidade_cano;

                // Movimentação vertical alternada dos canos superiores e inferiores
                if (cano.subir) {
                    cano.y -= 2;
                    if (cano.y < 50) {
                        cano.subir = false;
                    }
                } else {
                    cano.y += 2;
                    if (cano.y + intervalo_canos > altura_tela - 50) {
                        cano.subir = true;
                    }
                }
            }

            // Adiciona novos canos quando o último cano estiver a uma certa distância da tela
            if (canos.back().x < largura_tela - intervalo_canos) {
                int novo_y = rand() % (altura_tela - intervalo_canos - 150);
                if (pontuacao <= 15) {
                    novo_y += 50; // Abaixar a abertura do cano
                }
                canos.push_back({largura_tela, novo_y, true});
            }

            // Remove canos que saíram da tela
            if (!canos.empty() && canos.front().x + largura_cano < 0) {
                canos.erase(canos.begin());
                pontuacao++;
            }

            // Colisões
            for (const auto& cano : canos) {
                if (x_pássaro + largura_pássaro > cano.x && x_pássaro < cano.x + largura_cano) {
                    if (y_pássaro < cano.y || y_pássaro + altura_pássaro > cano.y + intervalo_canos) {
                        vidas--;
                        Mix_PlayChannel(-1, som_colisao, 0);
                        if (vidas <= 0) {
                            game_over = true;
                            break;
                        }
                    }
                }
            }

            // Adiciona novas bolhas
            if (rand() % 100 < 2) {
                int novo_x_bolha = rand() % (largura_tela - largura_bolha);
                bolhas.push_back({novo_x_bolha, 0});
            }

            // Movimenta bolhas e verifica colisões
            for (auto it = bolhas.begin(); it != bolhas.end(); ) {
                it->y += 5; // Velocidade alta das bolhas

                if (it->y > altura_tela) {
                    it = bolhas.erase(it);
                } else if (x_pássaro + largura_pássaro > it->x && x_pássaro < it->x + largura_bolha &&
                           y_pássaro + altura_pássaro > it->y && y_pássaro < it->y + altura_bolha) {
                    vidas += 5; // Aumenta 5 vidas ao colidir com bolha
                    it = bolhas.erase(it);
                } else {
                    ++it;
                }
            }

            // Ajusta dificuldade conforme a pontuação
            if (pontuacao >= 20) {
                velocidade_cano = 3; // Velocidade média
                intervalo_canos = 200; // Diminuir espaço entre os canos
            }

            atualizarTela(pontuacao, vidas, x_pássaro, y_pássaro, intervalo_canos, game_over);
        } else {
            atualizarTela(pontuacao, vidas, x_pássaro, y_pássaro, intervalo_canos, game_over);
            SDL_Delay(2000);
        }

        SDL_Delay(16); // Aproximadamente 60 FPS
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        return -1;
    }

    window = SDL_CreateWindow("Flappy Bird", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, largura_tela, altura_tela, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Quit();
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (TTF_Init() == -1) {
        Mix_CloseAudio();
        IMG_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    carregarRecursos();
    jogo();

    TTF_CloseFont(font);
    TTF_CloseFont(font_game_over);
    TTF_CloseFont(font_iniciar);
    Mix_FreeChunk(som_pulo);
    Mix_FreeChunk(som_colisao);
    SDL_DestroyTexture(imagem_fundo);
    SDL_DestroyTexture(imagem_cano);
    SDL_DestroyTexture(imagem_pássaro);
    SDL_DestroyTexture(imagem_bolha);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
