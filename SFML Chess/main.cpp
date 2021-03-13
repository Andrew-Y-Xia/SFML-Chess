#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>

#define WIDTH 1024


// Global vars:
sf::Texture textures[13];


// What type of piece is it?
enum piece_type {
    Empty,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

// info for castling states for one side
enum castling_privelege {
    queenside,
    kingside,
    both,
    none
};

// container for indiviual squares on chess board
struct Square {
    piece_type piece: 4;
    // 0 is white, 1 is black
    unsigned int color: 1;
    unsigned int enpassant_available: 1;
};



class Board {
private:
    Square squares[8][8];
    
    // true is white, false is black
    bool current_turn;
    
    castling_privelege black_castle_privelege;
    castling_privelege white_castle_privelege;
    
    int halfmove_counter;
    int fullmove_counter;
    
public:
    Board() {
        
    }
    
    
    void render_board(sf::RenderWindow* window) {
        int addon;
        Square current_square;
        sf::Sprite sprite;
        
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                addon = 0;
                current_square = squares[y][x];

                if (current_square.color) {
                    addon = 6;
                }
                
                switch (current_square.piece) {
                    case Bishop:
                        sprite.setTexture(textures[0 + addon]);
                    case King:
                        sprite.setTexture(textures[1 + addon]);
                    case Knight:
                        sprite.setTexture(textures[2 + addon]);
                    case Pawn:
                        sprite.setTexture(textures[3 + addon]);
                    case Queen:
                        sprite.setTexture(textures[4 + addon]);
                    case Rook:
                        sprite.setTexture(textures[5 + addon]);
                    case Empty:
                        sprite.setTexture(textures[12]);
                }
            }
        }
    }
};



void load_textures() {
    
    std::string str("bknpqr");
    std::string::iterator it = str.begin();
    std::string str2;
    
    for (int i = 0; i < 6; i++) {
        sf::Texture texture;
        str2 = "Resources/Chess_";
        
        texture.loadFromFile(str2.append(1, *it) + "lt60.png");
        textures[i] = texture;
        
        str2 = "Resources/Chess_";
        texture.loadFromFile(str2.append(1, *it) + "dt60.png");
        textures[i+6] = texture;
        
        ++it;
    }
    
    sf::Texture texture;
    texture.create(60, 60);
    textures[12] = texture;
}


int main()
{
    // create the window
    std::cout << std::__fs::filesystem::current_path() << std::endl;
    
    sf::RenderWindow window(sf::VideoMode(WIDTH, WIDTH), "My window");
    load_textures();

    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...
        
        // Draw the Grid
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                // init rectangle
                sf::RectangleShape tempRect(sf::Vector2f(WIDTH/8, WIDTH/8));
                
                // Make checkerboard pattern
                if ((x + y) % 2 == 0) {
                    tempRect.setFillColor(sf::Color(150, 50, 250));
                }
                else {
                    tempRect.setFillColor(sf::Color(150, 50, 200));
                }
                
                // move to proper location and draw
                tempRect.move(x * WIDTH/8, y * WIDTH/8);
                window.draw(tempRect);
            }
        }
        
        // Draw the pieces
        
        
        
        // end the current frame
        window.display();
    }

    return EXIT_SUCCESS;
}
