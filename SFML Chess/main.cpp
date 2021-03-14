#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <ctype.h>
#include <vector>

#define WIDTH 1024
#define SCALE 1.3
#define OFFSET 2


// Global vars:
sf::Texture textures[13];
std::vector<sf::Sprite> sprites;


struct Cords {
    int x, y;
};


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


// container for indiviual squares on chess board
struct Square {
    piece_type piece;
    // 0 is white, 1 is black
    unsigned int color: 1;
    unsigned int enpassant_available: 1;
};



class Board {
private:
    Square squares[8][8];
    
    // true is white, false is black
    bool current_turn;
   
    
    bool white_can_castle_queenside;
    bool white_can_castle_kingside;
    bool black_can_castle_queenside;
    bool black_can_castle_kingside;
    
    int halfmove_counter;
    int fullmove_counter;
    
public:
    Board() {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                Square current_square;
                current_square.piece = Empty;
                current_square.color = 0;
                current_square.enpassant_available = 0;
            }
        }
    }
    
    Board(std::string str) {
        read_LEN(str);
    }
    
    
    void set_texture_to_pieces() {
        int addon;
        Square current_square;
        
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                addon = 0;
                current_square = squares[y][x];

                if (current_square.color) {
                    addon = 6;
                }
                if (current_square.piece != Empty) {
                    
                    sf::Sprite sprite;
                    
                    switch (current_square.piece) {
                        case Bishop:
                            sprite.setTexture(textures[0 + addon]);
                            break;
                        case King:
                            sprite.setTexture(textures[1 + addon]);
                            break;
                        case Knight:
                            sprite.setTexture(textures[2 + addon]);
                            break;
                        case Pawn:
                            sprite.setTexture(textures[3 + addon]);
                            break;
                        case Queen:
                            sprite.setTexture(textures[4 + addon]);
                            break;
                        case Rook:
                            sprite.setTexture(textures[5 + addon]);
                            break;
                        case Empty:
                            break;
                    }
                    
                    
                    
                    sprite.setOrigin(sf::Vector2f(30, 30));
                    sprite.setPosition(x * WIDTH/8 + WIDTH/16 - OFFSET, y * WIDTH/8 + WIDTH/16 - OFFSET);
                    sprite.setScale(sf::Vector2f(SCALE, SCALE));
                    sprites.push_back(sprite);
                    
                }
//                std::cout << x << ", " << y << '\n';
            }
        }
    }
    
    
    void read_LEN(std::string str) {
        int state_flag = 0;
        
        std::string en_passant_square, halfmove_str, fullmove_str;

        int x = 0;
        int y = 0;
        
        for (std::string::iterator it=str.begin(); it!=str.end(); ++it) {
            if (*it == ' ') {
                state_flag += 1;
            }
            else if (state_flag == 0) {
                if (*it == '/') {
                    x = 0;
                    y += 1;
//                    std::cout << "y increment" << '\n';
                }
                else {
                    if (isdigit(*it)) {
                        int blanks = *it - '0';
                        for (int i = 0; i < blanks; i++) {
                            Square square;
                            square.color = 0;
                            square.piece = Empty;
                            square.enpassant_available = 0;
                            squares[y][x] = square;
                            x++;
                        }
                    }
                    else {
                        Square square;
                        square.enpassant_available = 0;
                        if (isupper(*it)) {
                            square.color = 0;
                        }
                        else {
                            square.color = 1;
                        }
                        
                        switch ((char) tolower(*it)) {
                            case 'r':
                                square.piece = Rook;
                                break;
                            case 'b':
                                square.piece = Bishop;
                                break;
                            case 'n':
                                square.piece = Knight;
                                break;
                            case 'k':
                                square.piece = King;
                                break;
                            case 'q':
                                square.piece = Queen;
                                break;
                            case 'p':
                                square.piece = Pawn;
                                break;
                            default:
                                std::cout << "This should not have been reached. Invalid piece: " << (char) tolower(*it) <<'\n';
                        }
                        squares[y][x] = square;
                        // std::cout << squares[y][x].piece << '\n';
                        x++;
                    }
                }
            }
            else if (state_flag == 1) {
                if (*it == 'w') {
                    current_turn = true;
                }
                else {
                    current_turn = false;
                }
            }
            else if (state_flag == 2) {
                if (*it == '-') {
                    white_can_castle_queenside = false;
                    white_can_castle_kingside = false;
                    black_can_castle_queenside = false;
                    black_can_castle_kingside = false;
                }
                else if (*it == 'K') {
                    white_can_castle_kingside = true;
                }
                else if (*it == 'Q') {
                    white_can_castle_queenside = true;
                }
                else if (*it == 'k') {
                    black_can_castle_kingside = true;
                }
                else if (*it == 'q') {
                    black_can_castle_queenside = true;
                }
            }
            else if (state_flag == 3) {
                en_passant_square.append(1, *it);
            }
            else if (state_flag == 4) {
                halfmove_str.append(1, *it);
            }
            else if (state_flag == 5) {
                fullmove_str.append(1, *it);
            }
            else {
                std::cout << "This state should not have been reached. ReadLEN Error occured." << '\n';
            }
        }
        
        
        halfmove_counter = std::stoi(halfmove_str);
        fullmove_counter = std::stoi(fullmove_str);

        if (en_passant_square[0] != '-') {
            switch (en_passant_square[0]) {
                case 'a':
                    x = 0;
                    break;
                case 'b':
                    x = 1;
                    break;
                case 'c':
                    x = 2;
                    break;
                case 'd':
                    x = 3;
                    break;
                case 'e':
                    x = 4;
                    break;
                case 'f':
                    x = 5;
                    break;
                case 'g':
                    x = 6;
                    break;
                case 'h':
                    x = 7;
                    break;
                default:
                    std::cout << "Should not have been reached. En Passant square cords are wrong";
            }
            y = 7 - (en_passant_square[1] - '0');

            squares[y][x].enpassant_available = 1;
        }
    }
    
    void debug_print() {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                std::cout << squares[y][x].piece << " " << squares[y][x].color << '\n';
            }
        }
    }
};



void draw_pieces(sf::RenderWindow* window) {
    for (std::vector<sf::Sprite>::iterator it = sprites.begin() ; it != sprites.end(); ++it) {
        window->draw(*it);
    }
}



sf::Sprite* locate_sprite_clicked(int x, int y) {
    for (std::vector<sf::Sprite>::iterator it = sprites.begin() ; it != sprites.end(); ++it) {
        if ((*it).getGlobalBounds().contains(x, y)) {
            return (&(*it));
        }
    }
    return NULL;
}


Cords find_grid_bounds(int x, int y) {
    Cords c;
    if (!(0 <= x && x <= WIDTH && 0 <= y && y <= WIDTH)) {
        c.x = -1;
        c.y = -1;
    } else {
        c.x = (x * 8) / WIDTH;
        c.y = (y * 8) / WIDTH;
    }
    return c;
}


bool is_move_valid(int x, int y) {
    
    
    if (0 <= x && x <= 7 && 0 <= y && y <= 7) {
        return true;
    }
    
//    std::cout << x << " " << y << std::endl;
    
    return false;
}




void load_textures() {
    
    std::string str("bknpqr");
    std::string::iterator it = str.begin();
    std::string str2;
    sf::Texture texture;
    
    for (int i = 0; i < 6; i++) {
        str2 = "Resources/Chess_";
        
        texture.loadFromFile(str2.append(1, *it) + "lt60.png");
        texture.setSmooth(true);
        textures[i] = texture;
        
        str2 = "Resources/Chess_";
        texture.loadFromFile(str2.append(1, *it) + "dt60.png");
        texture.setSmooth(true);
        textures[i+6] = texture;
        
        ++it;
    }
    
    sf::Texture blank;
    blank.create(60, 60);
    textures[12] = blank;
}


int main()
{
    struct sprite_drag_data {
        sf::Sprite* sprite;
        int x, y;
    };
    sprite_drag_data sprite_being_dragged;
    sprite_being_dragged.sprite = NULL;

    std::cout << std::__fs::filesystem::current_path() << std::endl;
    
    // create the window
    sf::RenderWindow window(sf::VideoMode(WIDTH, WIDTH), "My window");
    
    sprites.reserve(64);
    load_textures();
    
    // init the chess board
    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
//    board.debug_print();
    
    board.set_texture_to_pieces();
    
    
    sf::RectangleShape displaygrid[8][8];
    // create the Grid
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // init rectangle
            sf::RectangleShape tempRect(sf::Vector2f(WIDTH/8, WIDTH/8));
            
            // Make checkerboard pattern
            if ((x + y) % 2 == 0) {
                tempRect.setFillColor(sf::Color(24, 184, 9));
            }
            else {
                tempRect.setFillColor(sf::Color(4, 145, 16));
            }
            
            // move to proper location and draw
            tempRect.setPosition(x * WIDTH/8, y * WIDTH/8);
            displaygrid[y][x] = tempRect;
        }
    }
    

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
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left && !sprite_being_dragged.sprite)
                {
                    sprite_being_dragged.sprite = locate_sprite_clicked(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                    
                    if (sprite_being_dragged.sprite) {
                        Cords c = find_grid_bounds(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                        sprite_being_dragged.x = c.x;
                        sprite_being_dragged.y = c.y;
                    }
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (sprite_being_dragged.sprite) {
    
                        Cords c = find_grid_bounds(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                        if (is_move_valid(c.x, c.y)) {
                            sprite_being_dragged.sprite->setPosition(c.x * WIDTH/8 + WIDTH/16 - OFFSET, c.y * WIDTH/8 + WIDTH/16 - OFFSET);
                        }
                        else {
                            sprite_being_dragged.sprite->setPosition(sprite_being_dragged.x * WIDTH/8 + WIDTH/16 - OFFSET, sprite_being_dragged.y * WIDTH/8 + WIDTH/16 - OFFSET);
                        }
                        
                        sprite_being_dragged.sprite = NULL;
                    }
                }
            }
        }

        
        // Do Calcs/Pos changes
        
        if (sprite_being_dragged.sprite) {
            sprite_being_dragged.sprite->setPosition(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
        }
        
        
        
        
        // clear the window with black color
        window.clear(sf::Color::Black);

        // Draw stage
        
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                window.draw(displaygrid[y][x]);
            }
        }
        
        // Draw the pieces
        draw_pieces(&window);
        
        
        
        
        
        // end the current frame
        window.display();
    }

    return EXIT_SUCCESS;
}
