#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <ctype.h>
#include <forward_list>
#include <sparsehash/dense_hash_map>

#include "ResourcePath.hpp"


#define WIDTH 1024
#define SCALE 1.3
#define OFFSET 2
#define ASDF std::cout << "asdf" << std::endl


// Global vars:
sf::Texture textures[13];
std::forward_list<sf::Sprite> sprites;


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


enum move_type {
    // Pawn moving two steps for initial move is not included since it can be treated like a normal move
    Normal,
    Promote_to_Queen,
    Promote_to_Rook,
    Promote_to_Bishop,
    Promote_to_Knight,
    En_Passant,
    Castle_Queenside,
    Castle_Kingside,
    Illegal
};


struct Move {
    Cords from_c;
    Cords to_c;
    
    move_type type;
};


// container for indiviual squares on chess board
struct Square {
    piece_type piece: 7;
    // 0 is white, 1 is black
    unsigned int color: 1;
};


// Math function: sign
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}


class Board {
private:
    Square squares[8][8];
    
    // 0 is white, 1 is black
    int current_turn;
   
    
    bool white_can_castle_queenside;
    bool white_can_castle_kingside;
    bool black_can_castle_queenside;
    bool black_can_castle_kingside;
    
    int halfmove_counter;
    int fullmove_counter;
    
    Cords en_passant_cords;
    
    // Attacked by enemy, based off turn
    std::forward_list<Cords> attacked_squares;
    
public:
    Board() {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                Square current_square;
                current_square.piece = Empty;
                current_square.color = 0;
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
                    sprites.push_front(sprite);
                    
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
                            squares[y][x] = square;
                            x++;
                        }
                    }
                    else {
                        Square square;
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
                    current_turn = 0;
                }
                else {
                    current_turn = 1;
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
            
            Cords c;
            c.x = x; c.y = y;

            en_passant_cords = c;
        }
    }
    
    void debug_print() {
        for (int i = 0; i < 60; i++) {
            std::cout << std::endl;
        }
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                char c;
                switch (squares[y][x].piece) {
                    case Empty:
                        c = '0';
                        break;
                    case Pawn:
                        c = 'p';
                        break;
                    case Knight:
                        c = 'n';
                        break;
                    case Bishop:
                        c = 'b';
                        break;
                    case Rook:
                        c = 'r';
                        break;
                    case Queen:
                        c = 'q';
                        break;
                    case King:
                        c = 'k';
                        break;
                }
                if (squares[y][x].color == 0 && squares[y][x].piece != Empty) {
                    c = (char) toupper(c);
                }
                std::cout << c << ' ';
            }
            std::cout << std::endl;
        }
    }
    
    Cords sliding_pieces_incrementer(int x, int y, int increment_x, int increment_y) {
        Cords c;
        c.x = x;
        c.y = y;
        
        do {
            c.x += increment_x;
            c.y += increment_y;
            
            
            if (!is_within_bounds(c.x, c.y)) {
                c.x -= increment_x;
                c.y -= increment_y;
                return c;
            }
            else if (squares[c.y][c.x].piece != Empty) {
                if (squares[c.y][c.x].color != current_turn) {
                    return c;
                }
                else {
                    c.x -= increment_x;
                    c.y -= increment_y;
                    return c;
                }
            }
            
        } while (1);
    }
    
    
    void generate_attacked_squares();
    
    
    bool is_square_under_attack(int attacker, int x, int y) {
        // TODO: find if square is being attacked.
        return false;
    }
    
    bool is_friendly_piece(int x, int y) {
        if (squares[y][x].piece != Empty && squares[y][x].color == current_turn) {
            return true;
        }
        else {
            return false;
        }
    }
    
    
    bool is_pawn_move_valid(Move move, Move& return_move) {
        // TODO: En Passant + First move
        if (squares[move.from_c.y][move.from_c.x].color == 1) {
            if (move.to_c.y - move.from_c.y == 1 && move.to_c.x == move.from_c.x && squares[move.to_c.y][move.to_c.x].piece == Empty) {
                return true;
            }
            else if (abs(move.from_c.x - move.to_c.x) == 1 && move.to_c.y - move.from_c.y == 1 && squares[move.to_c.y][move.to_c.x].piece != Empty) {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            if (move.to_c.y - move.from_c.y == -1 && move.to_c.x == move.from_c.x && squares[move.to_c.y][move.to_c.x].piece == Empty) {
                return true;
            }
            else if (abs(move.from_c.x - move.to_c.x) == 1 && move.to_c.y - move.from_c.y == -1 && squares[move.to_c.y][move.to_c.x].piece != Empty) {
                return true;
            }
            else {
                return false;
            }
        }
    }
    
    bool is_king_move_valid(Move move, Move& return_move) {
        // TODO: Castling
        if (!(abs(move.from_c.y - move.to_c.y) <= 1 && abs(move.from_c.x - move.to_c.x) <= 1)) {
            return false;
        }
        else if (is_square_under_attack(!current_turn, move.to_c.x, move.to_c.y)) {
            return false;
        }
        else {
            return true;
        }
    }
    
    bool is_knight_move_valid(int from_x, int from_y, int to_x, int to_y) {
        return ((abs(from_x - to_x) == 1 && (abs(from_y - to_y) == 2)) || (abs(from_x - to_x) == 2 && (abs(from_y - to_y) == 1)));
    }
    
    
    bool sliding_path_check(int from_x, int from_y, int to_x, int to_y) {
        Cords c = sliding_pieces_incrementer(from_x, from_y, sgn<int>(to_x-from_x), sgn<int>(to_y-from_y));
        if (!(abs(from_x - c.x) >= abs(from_x - to_x) && abs(from_y - c.y) >= abs(from_y - to_y))) {
            return false;
        }
        return true;
    }
    
    bool is_bishop_move_valid(int from_x, int from_y, int to_x, int to_y) {
        if (abs(from_x - to_x) != abs(from_y - to_y)) {
            return false;
        }
        else {
            return sliding_path_check(from_x, from_y, to_x, to_y);
        }
    }
    
    bool is_rook_move_valid(int from_x, int from_y, int to_x, int to_y) {
        if (!(from_x - to_x == 0 || from_y - to_y == 0)) {
            return false;
        }
        else {
            return sliding_path_check(from_x, from_y, to_x, to_y);
        }
    }
    
    bool is_queen_move_valid(int from_x, int from_y, int to_x, int to_y) {
        if (abs(from_x - to_x) != abs(from_y - to_y) && !(from_x - to_x == 0 || from_y - to_y == 0)) {
            return false;
        }
        else {
            return sliding_path_check(from_x, from_y, to_x, to_y);
        }
    }
    
    
    bool is_correct_turn(int x, int y) {
        return (squares[y][x].color == current_turn);
    }
    
    bool is_within_bounds(int x, int y) {
        return (0 <= x && x <= 7 && 0 <= y && y <= 7);
    }
    
    // TODO: Pins
    bool piece_is_pinned(int x, int y);
    
    bool is_following_piece_rules(Move move, Move& return_move) {
        
        switch (squares[move.from_c.y][move.from_c.x].piece) {
            // Pawn and king are special cases, handle them differently
                
            case Pawn:
                return is_pawn_move_valid(move, return_move);
                break;
            case King:
                return is_king_move_valid(move, return_move);
                break;
                
            default:
                // All the other pieces
                switch (squares[move.from_c.y][move.from_c.x].piece) {
                case Knight:
                    return is_knight_move_valid(move.from_c.x, move.from_c.y, move.to_c.x, move.to_c.y);
                    break;
                case Bishop:
                    return is_bishop_move_valid(move.from_c.x, move.from_c.y, move.to_c.x, move.to_c.y);
                    break;
                case Rook:
                    return is_rook_move_valid(move.from_c.x, move.from_c.y, move.to_c.x, move.to_c.y);
                    break;
                case Queen:
                    return is_queen_move_valid(move.from_c.x, move.from_c.y, move.to_c.x, move.to_c.y);
                    break;
                default:
                    std::cout << "Something really bad must have happened to get here" << std::endl;
                    break;
            }
        }
        
        return false;
    }
    
    Move is_move_valid(Move move) {
        Move return_move;
        return_move = move;
        return_move.type = Normal;
        
        if (!(is_correct_turn(move.from_c.x, move.from_c.y))) {
            return_move.type = Illegal;
        }
        else if (!(is_within_bounds(move.to_c.x, move.to_c.y))) {
            return_move.type = Illegal;
        }
        else if (move.from_c.x == move.to_c.x && move.from_c.y == move.to_c.y){
            return_move.type = Illegal;
        }
        else if (is_friendly_piece(move.to_c.x, move.to_c.y)) {
            return_move.type = Illegal;
        }
        else if (!is_following_piece_rules(move, return_move)) {
            return_move.type = Illegal;
        }
        
    //    std::cout << x << " " << y << std::endl;
        return return_move;
    }
    
    
    void move(Move move) {
        squares[move.to_c.y][move.to_c.x] = squares[move.from_c.y][move.from_c.x];
        Square square;
        square.piece = Empty;
        square.color = 0;
        squares[move.from_c.y][move.from_c.x] = square;
        
        current_turn = !current_turn;
        
//        generate_attacked_squares();
    }
    
    Move request_move(Move move);
        // This functions takes in a move requested by the board, and returns the correct type of it is,
        // whilst updating the internal board appropriately. It checks if the move is illegal, and whether it is a special move

};



void draw_pieces(sf::RenderWindow* window) {
    for (std::forward_list<sf::Sprite>::iterator it = sprites.begin() ; it != sprites.end(); ++it) {
        window->draw(*it);
    }
}



sf::Sprite* locate_sprite_clicked(int x, int y) {
    for (std::forward_list<sf::Sprite>::iterator it = sprites.begin() ; it != sprites.end(); ++it) {
        if ((*it).getGlobalBounds().contains(x, y)) {
            return (&(*it));
        }
    }
    return NULL;
}



int locate_sprite_clicked_index(int x, int y, sf::Sprite* sprite) {
    int counter = 0;
    for (std::forward_list<sf::Sprite>::iterator it = sprites.begin() ; it != sprites.end(); ++it) {
        if ((*it).getGlobalBounds().contains(x, y) && (&(*it) != sprite)) {
//            std::cout << counter << std::endl;
            return counter;
        }
        counter++;
    }
    return -1;
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




void load_textures() {
    
    std::string str("bknpqr");
    std::string::iterator it = str.begin();
    std::string str2;
    sf::Texture texture;
    
    for (int i = 0; i < 6; i++) {
        str2 = resourcePath() + "Chess_";
        
        texture.loadFromFile(str2.append(1, *it) + "lt60.png");
        texture.setSmooth(true);
        textures[i] = texture;
        
        str2 = resourcePath() + "Chess_";
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


    // create the window
    sf::RenderWindow window(sf::VideoMode(WIDTH, WIDTH), "My window");
    
    load_textures();
    
    // init the chess board
    Board board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
//    Board board("8/8/8/8/8/k7/pK6/8 b KQkq - 0 1");

    board.set_texture_to_pieces();
    
    Move move;
    
    
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
                    // save the sprite being dragged
                    sprite_being_dragged.sprite = locate_sprite_clicked(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                    
                    if (sprite_being_dragged.sprite) {
                        move.from_c = find_grid_bounds(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                   }
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (sprite_being_dragged.sprite) {
    
                        move.to_c = find_grid_bounds(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                        
                        Move return_move = board.is_move_valid(move);
                        
                        
                        // This section checks and handles the validity of move, including drawing the sprite
                        if (return_move.type != Illegal) {
                            // If move is valid, set the sprite to the new position, delete the sprite that was residing in the to_location, and register the move with the board.
                            int temp = locate_sprite_clicked_index(return_move.to_c.x * WIDTH/8 + WIDTH/16 - OFFSET, return_move.to_c.y * WIDTH/8 + WIDTH/16 - OFFSET, sprite_being_dragged.sprite);
                            
//                            std::cout << c.x << ' ' << c.y << std::endl;
                            sprite_being_dragged.sprite->setPosition(return_move.to_c.x * WIDTH/8 + WIDTH/16 - OFFSET, return_move.to_c.y * WIDTH/8 + WIDTH/16 - OFFSET);
                            
                            
                            if (temp != -1) {
                                std::forward_list<sf::Sprite>::iterator it = sprites.before_begin();
                                std::advance(it, temp);
                                sprites.erase_after(it);
                            }
                            
                            
                            board.move(move);
//                            board.debug_print();
                        }
                        else {
                            // If move isn't valid, return sprite to original position and do nothing
                            sprite_being_dragged.sprite->setPosition(return_move.from_c.x * WIDTH/8 + WIDTH/16 - OFFSET, return_move.from_c.y * WIDTH/8 + WIDTH/16 - OFFSET);
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
