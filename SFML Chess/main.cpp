#include <SFML/Graphics.hpp>
#include <iostream>
#include <ctime>
#include <string>
#include <ctype.h>
#include <forward_list>
#include <sparsehash/dense_hash_map>

#include "ResourcePath.hpp"


#define WIDTH 1024
#define SCALE 1.3
#define OFFSET 2
#define ASDF std::cout << "asdf" << std::endl

#define PAWN_VALUE 10
#define KNIGHT_VALUE 30
#define BISHOP_VALUE 33
#define ROOK_VALUE 50
#define QUEEN_VALUE 90


// Global vars:
sf::Texture textures[13];
std::forward_list<sf::Sprite> sprites, promotion_sprites_white, promotion_sprites_black;
int incre8[8];

struct Cords {
    int x: 8;
    int y: 8;
    
    bool operator==(const Cords c2) {
        return (this->x == c2.x && this->y == c2.y);
    }
    bool operator!=(const Cords c2) {
        return !(*this == c2);
    }
    Cords() {
        x = -1;
        y = -1;
    }
    Cords(int a, int b) {
        x = a; y = b;
    }
};

 
namespace std
{
    template<>
    struct hash<Cords>
    {
        std::size_t operator()(Cords const& c) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(c.x);
            std::size_t h2 = std::hash<int>{}(c.y);
            size_t seed = 42;
            seed ^= h1 + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);
            seed ^= h2 + 0x9e3779b97f4a7c15 + (seed << 6) + (seed >> 2);; // or use h1 ^ (h2 << 1)
            return seed;
        }
    };
}

struct cords_eq {
  bool operator()(Cords c1, Cords c2) const {
    return (c1.x == c2.x && c1.y == c2.y);
  }
};

struct eqstr
{
  bool operator()(std::string s1, std::string s2) const
  {
    return (s1 == s2);
  }
};

template <class T, class Sub>
bool lookup(T Set, Sub word) {
    typename T::const_iterator it = Set.find(word);
    return it != Set.end();
}


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
    
    move_type type: 4;
    int score: 8;
};


bool move_cmp(Move first, Move second) {
    return first.score > second.score;
}


struct Move_data {
    Move move;
    piece_type captured_piece : 4;
    Cords previous_en_passant_cords;
    bool white_can_castle_queenside: 1;
    bool white_can_castle_kingside: 1;
    bool black_can_castle_queenside: 1;
    bool black_can_castle_kingside: 1;
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

void set_single_texture(int color, piece_type piece, sf::Sprite& sprite) {
    int addon;
    if (color == 0) {
        addon = 0;
    }
    else {
        addon = 6;
    }
    switch (piece) {
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
}

void debug_print_moves(std::forward_list<Move> moves) {
    for (std::forward_list<Move>::iterator it = moves.begin() ; it != moves.end(); ++it) {
        std::cout << "From: " << it->from_c.x << ", " << it->from_c.y << "  |  To: " << it->to_c.x << ", " << it->to_c.y << "  |  Type: " << it->type << std::endl;
    }
    for (int i = 0; i < 10; i++) {
        std::cout << '\n';
    }
}

int increment_to_index(Cords c) {
    int i = 3*(c.x + 1) + (c.y+1);
    if (i > 4) {
        i--;
    }
    return i;
}

Cords index_to_increment(int i) {
    switch (i) {
        case 0:
            return Cords{-1, -1};
        case 1:
            return Cords{-1, 0};
        case 2:
            return Cords{-1, 1};
        case 3:
            return Cords{0, -1};
        case 4:
            return Cords{0, 1};
        case 5:
            return Cords{1, -1};
        case 6:
            return Cords{1, 0};
        case 7:
            return Cords{1, 1};
        default:
            std::cout << "Should not have occured index_to_increment\n";
    }
}


class Board {
private:
    Square squares[8][8];
    
    // 0 is white, 1 is black
    int current_turn;
   
    
    bool white_can_castle_queenside: 1;
    bool white_can_castle_kingside: 1;
    bool black_can_castle_queenside: 1;
    bool black_can_castle_kingside: 1;
    
    int halfmove_counter;
    int fullmove_counter;
    
    Cords en_passant_cords, black_king_loc, white_king_loc;
    
    google::dense_hash_map<Cords, Cords, std::hash<Cords>, cords_eq> pinned_by, pinning;
    
    std::forward_list<Move> legal_moves;
    
    google::dense_hash_map<std::string, int, std::hash<std::string>, eqstr> previous_board_positions;
    
    std::forward_list<Cords> attacks_on_the_king;
    
    std::forward_list<Move_data> move_stack;
    
    int black_piece_values, white_piece_values;
    
public:
    Board() {
        standard_setup();
        read_FEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        post_readLEN();
    }
    
    void find_kings() {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (squares[y][x].piece == King) {
                    if (squares[y][x].color == 1) {
                        black_king_loc = Cords{x, y};
                    }
                    else {
                        white_king_loc = Cords{x, y};
                    }
                }
            }
        }
    }
    
    void standard_setup() {
        Cords n = {-1, -1};
        Cords m = {-2, -2};
        pinned_by.set_empty_key(n);
        pinning.set_empty_key(n);
        
        pinned_by.set_deleted_key(m);
        pinning.set_deleted_key(m);
        
        previous_board_positions.set_empty_key(std::string());
    }
    
    void post_readLEN() {
        find_kings();
        generate_pins(0, incre8);
        generate_pins(1, incre8);
//        generate_moves(legal_moves);
        reset_piece_values();
    }
    
    Board(std::string str) {
        standard_setup();
        read_FEN(str);
        post_readLEN();
    }
    
    int get_current_turn() {
        return current_turn;
    }
    
    int piece_to_value(piece_type piece) {
        switch (piece) {
            case Empty:
            case King:
                return 0;
            case Pawn:
                return PAWN_VALUE;
            case Knight:
                return KNIGHT_VALUE;
            case Bishop:
                return BISHOP_VALUE;
            case Rook:
                return ROOK_VALUE;
            case Queen:
                return QUEEN_VALUE;
        }
    }
    
    void reset_piece_values() {
        white_piece_values = 0;
        black_piece_values = 0;
        
        
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (squares[y][x].color == 1) {
                    black_piece_values += piece_to_value(squares[y][x].piece);
                }
                else {
                    white_piece_values += piece_to_value(squares[y][x].piece);
                }
            }
        }
    }
    
    void set_texture_to_pieces() {
        Square current_square;
        
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                current_square = squares[y][x];
                
                if (current_square.piece != Empty) {
                    
                    sf::Sprite sprite;
                    
                    set_single_texture(current_square.color, current_square.piece, sprite);
                    
                    
                    sprite.setOrigin(sf::Vector2f(30, 30));
                    sprite.setPosition(x * WIDTH/8 + WIDTH/16 - OFFSET, y * WIDTH/8 + WIDTH/16 - OFFSET);
                    sprite.setScale(sf::Vector2f(SCALE, SCALE));
                    sprites.push_front(sprite);
                    
                }
//                std::cout << x << ", " << y << '\n';
            }
        }
    }
    
    char num_to_char(int input) {
        char c;
        switch(input) {
            case 0:
                c = 'a';
                break;
            case 1:
                c = 'b';
                break;
            case 2:
                c = 'c';
                break;
            case 3:
                c = 'd';
                break;
            case 4:
                c = 'e';
                break;
            case 5:
                c = 'f';
                break;
            case 6:
                c = 'g';
                break;
            case 7:
                c = 'h';
                break;
        }
        return c;
    }
    
    std::string generate_FEN() {
        std::string str;
        int running_counter = 0;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                char c = piece_type_to_char(squares[y][x].piece);
                if (squares[y][x].color == 0 && squares[y][x].piece != Empty) {
                    c = (char) toupper(c);
                }
                
                if (c == '0') {
                    running_counter += 1;
                }
                else if (running_counter != 0) {
                    str.append(std::to_string(running_counter));
                    running_counter = 0;
                }
                if (c != '0') {
                    str.append(1, c);
                }
            }
            if (running_counter != 0) {
                str.append(std::to_string(running_counter));
                running_counter = 0;
            }
            if (y != 7) {
                str.append(1, '/');
            }
        }
        str.append(1, ' ');
        if (current_turn == 1) {
            str.append(1, 'b');
        }
        else {
            str.append(1, 'w');
        }
        str.append(1, ' ');
        if (white_can_castle_kingside || white_can_castle_queenside || black_can_castle_kingside || black_can_castle_queenside) {
            if (white_can_castle_kingside) {
                str.append(1, 'K');
            }
            if (white_can_castle_queenside) {
                str.append(1, 'Q');
            }
            if (black_can_castle_kingside) {
                str.append(1, 'k');
            }
            if (black_can_castle_queenside) {
                str.append(1, 'q');
            }
        }
        else {
            str.append(1, '-');
        }
        str.append(1, ' ');
        
        if (en_passant_cords == Cords{-1, -1}) {
            str.append(1, '-');
        }
        else {
            char c = num_to_char(en_passant_cords.x);
            str.append(1, c);
            str.append(std::to_string(8 - en_passant_cords.y));
        }

        str.append(1, ' ');
        str.append(std::to_string(halfmove_counter));
        str.append(1, ' ');
        str.append(std::to_string(fullmove_counter));
        
        return str;
    }
    
    
    void read_FEN(std::string str) {
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
            y = 8 - (en_passant_square[1] - '0');
            
            en_passant_cords.x = x;
            en_passant_cords.y = y;
        }
        else {
            en_passant_cords.x = -1;
            en_passant_cords.y = -1;
        }
    }
    
    char piece_type_to_char(piece_type p) {
        switch (p) {
            case Empty:
                return '0';
            case Pawn:
                return 'p';
            case Knight:
                return 'n';
            case Bishop:
                return 'b';
            case Rook:
                return 'r';
            case Queen:
                return 'q';
            case King:
                return 'k';
        }
    }
    
    void debug_print() {
        for (int i = 0; i < 30; i++) {
            std::cout << std::endl;
        }
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                char c = piece_type_to_char(squares[y][x].piece);
                
                if (squares[y][x].color == 0 && squares[y][x].piece != Empty) {
                    c = (char) toupper(c);
                }
                std::cout << c << ' ';
            }
            std::cout << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "White can castle queenside: " << white_can_castle_queenside << std::endl;
        std::cout << "White can castle kingside: " << white_can_castle_kingside << std::endl;
        std::cout << "Black can castle queenside: " << black_can_castle_queenside << std::endl;
        std::cout << "Black can castle kingside: " << black_can_castle_kingside << std::endl;
        std::cout << std::endl;
        std::cout << "Enpassant cords: " << en_passant_cords.x << ' ' << en_passant_cords.y << std::endl;
    }
    
    Cords sliding_pieces_incrementer(int x, int y, int increment_x, int increment_y, bool ignore_king = false) {
        Cords c = {x, y};
        
        while (1) {
            c.x += increment_x;
            c.y += increment_y;
            
            
            if (!is_within_bounds(c.x, c.y)) {
                c.x -= increment_x;
                c.y -= increment_y;
                return c;
            }
            else if (squares[c.y][c.x].piece != Empty) {
                if (!(squares[c.y][c.x].piece == King && current_turn == squares[c.y][c.x].color && ignore_king)) {
                    return c;
                }
            }
        }
    }
    
    Cords ignore_square_incrementer(int x, int y, int increment_x, int increment_y, Cords ignore_squares[2]) {
        Cords c = {x, y};
        while (1) {
            c.x += increment_x;
            c.y += increment_y;
            
            
            if (!is_within_bounds(c.x, c.y)) {
                c.x -= increment_x;
                c.y -= increment_y;
                return c;
            }
            else if (squares[c.y][c.x].piece != Empty) {
                if (!(c == ignore_squares[0] || c == ignore_squares[1])) {
                    return c;
                }
            }
        }
    }
    
    void debug_attacked_squares(int attacker_color) {
        std::cout << '\n' << '\n' << '\n' << '\n';
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
//                is_square_under_attack(x, y, attacker_color);
                std::cout << is_square_under_attack(x, y, attacker_color) << ' ';
            }
            std::cout << std::endl;
        }
    }
    
    // Lots of boilerplate here
    void pin_slider(int x, int y, int increment_x, int increment_y, Cords& pinned_piece, Cords& pinning_piece ) {
        Cords c = {x, y};
        bool found_possible_pinned_piece = false;

        while (1) {
            c.x += increment_x;
            c.y += increment_y;
            
            
            if (!is_within_bounds(c.x, c.y)) {
                pinned_piece = Cords{-1, -1};
                pinning_piece = Cords{-1, -1};
                return;
            }
            // Test to see if piece is reached
            else if (squares[c.y][c.x].piece != Empty) {
                // First see if a possible pinned piece has not been found
                if (!found_possible_pinned_piece) {
                    // If it is the same color, piece may be pinned!
                    if (squares[y][x].color == squares[c.y][c.x].color) {
                        pinned_piece = c;
                        found_possible_pinned_piece = true;
                        continue;
                    }
                    // If it's not the same color, there can't be a pin no matter what
                    else {
                        pinned_piece = Cords{-1, -1};
                        pinning_piece = Cords{-1, -1};
                        return;
                    }
                }
                // If a possible pinned piece has already been found
                else {
                    // First check to see if the piece is same color. If it is, there cannot be a pin.
                    if (squares[y][x].color == squares[c.y][c.x].color) {
                        pinned_piece = Cords{-1, -1};
                        pinning_piece = Cords{-1, -1};
                        return;
                    }
                    // If the piece is of opposite color, check to see if it has the ability to pin (i.e bishops diagonal, rooks vert/hori)
                    switch (squares[c.y][c.x].piece) {
                        // Queen goes first since rook + bishop patterns = queen patterns
                        case Queen:
                            if (x == c.x || y == c.y || abs(c.y - y) == abs(c.x - x)) {
                                pinning_piece = c;
                                return;
                            }
                            else {
                                pinned_piece = Cords{-1, -1};
                                pinning_piece = Cords{-1, -1};
                                return;
                            }
                        case Bishop:
                            if (abs(c.y - y) == abs(c.x - x)) {
                                pinning_piece = c;
                                return;
                            }
                            else {
                                pinned_piece = Cords{-1, -1};
                                pinning_piece = Cords{-1, -1};
                                return;
                            }
                        case Rook:
                            if (x == c.x || y == c.y) {
                                pinning_piece = c;
                                return;
                            }
                            else {
                                pinned_piece = Cords{-1, -1};
                                pinning_piece = Cords{-1, -1};
                                return;
                            }
                        // Non slider pieces have no ability to pin
                        default:
                            pinned_piece = Cords{-1, -1};
                            pinning_piece = Cords{-1, -1};
                            return;
                    }
                }
            }
        }
    }
    
    void debug_pins() {
        for (auto it = pinning.begin(); it != pinning.end(); ++it) {
            std::cout << it->first.x << ' ' << it->first.y << " is pinning ";
            std::cout << it->second.x << ' ' << it->second.y << '\n';
        }
        std::cout << '\n';
        for (auto it = pinned_by.begin(); it != pinned_by.end(); ++it) {
            std::cout << it->first.x << ' ' << it->first.y << " is pinned by ";
            std::cout << it->second.x << ' ' << it->second.y << '\n';
        }
        std::cout << '\n' << '\n';
    }
    
    
    void delete_pins(int color, int* increments) {
        // Dont pass anything other than array size 8 to increments!!! Otherwise segfault
        for (auto it = pinning.begin(); it != pinning.end(); ++it) {
            // If the piece pinning is of opposite color, delete the pin data (deletion is neccesary otherwise iterator will iterate over it next time)
            // Also, look up the piece being pinned and delete this cord from the list
            
            if (squares[it->first.y][it->first.x].color != color && *(increments + increment_to_index(Cords{sgn(it->first.x - it->second.x), sgn(it->first.y - it->second.y)}))) {
                pinned_by.erase(it->second);
                /*
                 if (pinned_by[*(it->second.begin())].empty()) {
                 pinned_by.erase(*(it->second.begin()));
                 }
                 */
                pinning.erase(it->first);
            }
        }
    }
    
    void generate_pins(int color, int* increments) {
        // Dont pass anything other than array size 8 to increments!!! Otherwise segfault
        // Color 
        Cords king, pinned_piece, pinning_piece;
        
        king = color == 1 ? black_king_loc : white_king_loc;
        

        const int size = 8;
        Cords c;
        for (int i = 0; i < size; i++) {
            if (*(increments + i)) {
                c = index_to_increment(i);
                pin_slider(king.x, king.y, c.x, c.y, pinned_piece, pinning_piece);
                if (pinned_piece.x != -1) {
//                    std::cout << "Found pinned piece: " << pinned_piece.x << ' ' << pinned_piece.y << " | pinner: " << pinning_piece.x << ' ' << pinning_piece.y <<  '\n';
                    pinned_by[Cords{pinned_piece.x, pinned_piece.y}] = Cords{pinning_piece.x, pinning_piece.y};
                    pinning[Cords{pinning_piece.x, pinning_piece.y}] = Cords{pinned_piece.x, pinned_piece.y};
                }
            }
        }
        
//        debug_pins();
    }
    
    bool is_in_between(Cords c1, Cords c2, Cords move_to) {
        return (std::min(c1.x, c2.x) <= move_to.x && move_to.x <= std::max(c1.x, c2.x) && std::min(c1.y, c2.y) <= move_to.y && move_to.y <= std::max(c1.y, c2.y));
    }
    
    void print_attacks_on_king() {
        for (auto it = attacks_on_the_king.begin(); it != attacks_on_the_king.end(); ++it) {
            std::cout << it->x << ", " << it->y << std::endl;
        }
//        std::cout << white_king_loc.x << white_king_loc.y;
    }


    bool follows_check_rules(Move move) {
        
        Cords king_c = current_turn == 1 ? black_king_loc : white_king_loc;
        // If the king is being moved, then there's no need to do any checks cause the king's individual checks will make sure no illegal moves
        if (king_c == move.from_c) {
            return true;
        }
        
        // Check if in check
        if (!attacks_on_the_king.empty()) {
            // Iterate through the squares that are attacking king
            for (auto attacking_piece = attacks_on_the_king.begin(); attacking_piece != attacks_on_the_king.end(); ++attacking_piece) {
                if (move.type == En_Passant && move.from_c.y == attacking_piece->y && move.to_c.x == attacking_piece->x) {
                    continue;
                }
                if (!is_in_between(*attacking_piece, king_c, move.to_c)) {
                    return false;
                }
                // If slider piece, see if block-able
                switch (squares[attacking_piece->y][attacking_piece->x].piece) {
                    case Queen:
                        if (abs(move.to_c.y - attacking_piece->y) == abs(move.to_c.x - attacking_piece->x)) {
                            continue;
                        }
                        else if (attacking_piece->x == move.to_c.x && king_c.x == move.to_c.x) {
                            continue;
                        }
                        else if (attacking_piece->y == move.to_c.y && king_c.y == move.to_c.y) {
                            continue;
                        }
                        else {
                            return false;
                        }
                        break;
                    case Bishop:
                        if (abs(move.to_c.y - attacking_piece->y) != abs(move.to_c.x - attacking_piece->x)) {
                            return false;
                        }
                        break;
                    case Rook:
                        if (!(attacking_piece->x == move.to_c.x || attacking_piece->y == move.to_c.y)) {
                            return false;
                        }
                        break;
                    // If normal piece, see if taking the attacking piece
                    case Pawn:
                    case Knight:
                    case King:
                        if (move.to_c != *attacking_piece) {
                            return false;
                        }
                        break;
                    default:
                        std::cout << "should not have been reached follows_check_rules";
                }
            }
        }
        return true;
    }
    
    bool does_pass_basic_piece_checks(Move move, bool ignore_turns = false) {
        if (!(is_correct_turn(move.from_c.x, move.from_c.y)) && !ignore_turns) {
            return false;
        }
        else if (!(is_within_bounds(move.to_c.x, move.to_c.y))) {
            return false;
        }
        // Equality cords check
        else if (move.from_c.x == move.to_c.x && move.from_c.y == move.to_c.y){
            return false;
        }
        else if (is_friendly_piece(move.to_c.x, move.to_c.y) && !ignore_turns){
            return false;
        }
        else if (!follows_pin_rules(move)) {
            return false;
        }
        else if (!follows_check_rules(move)) {
            return false;
        }
        else {
            return true;
        }
    }
    
    void reset_attacks_on_the_king() {
        attacks_on_the_king.clear();
        
        Cords king_c = current_turn == 1 ? black_king_loc : white_king_loc;
        attacks_on_the_king = under_attack_cords(king_c.x, king_c.y, !current_turn);
    }
    
    int generate_moves(std::forward_list<Move>& moves, bool ignore_turns = false) {
        int moves_counter = 0;
        moves.clear();
        reset_attacks_on_the_king();
        

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                
                if (squares[y][x].color == current_turn) {
                    switch (squares[y][x].piece) {
    
                        case Empty:
                            break;
                        case Pawn:
                            moves_counter += generate_pawn_moves(moves, x, y, ignore_turns);
                            break;
                        case Knight:
                            moves_counter += generate_knight_moves(moves, x, y, ignore_turns);
                            break;
                        case Bishop:
                            moves_counter += generate_bishop_moves(moves, x, y, ignore_turns);
                            break;
                        case Rook:
                            moves_counter += generate_rook_moves(moves, x, y, ignore_turns);
                            break;
                        case Queen:
                            moves_counter += generate_queen_moves(moves, x, y, ignore_turns);
                            break;
                        case King:
                            moves_counter += generate_king_moves(moves, x, y, ignore_turns);
                            break;
                        default:
                            std::cout << "Should not have been reached at generate_moves." << std::endl;
                    }
                }
            }
        }
//        debug_print_moves(moves);
        return moves_counter;
    }
    
    bool calculate_en_passant_pins(const Cords &king_c, int x, int y, int incr_x, int incr_y) {

        Cords ignore_squares[2] = {Cords{x, y}, Cords{en_passant_cords.x, y}};
        Cords c = ignore_square_incrementer(king_c.x, king_c.y, incr_x, incr_y, ignore_squares);
        if (king_c.x == x && king_c.y == y) {
            std::cout << "Shouldn't occur calculate_en_passant_pins";
        }
        if (squares[c.y][c.x].color != current_turn) {
            switch (squares[c.y][c.x].piece) {
                case Empty:
                case Pawn:
                case Knight:
                case King:
                    break;
                case Queen:
                    if (c.y == king_c.y || abs(king_c.y - c.y) == abs(king_c.x - c.x)) {
                        return true;
                    }
                    break;
                case Bishop:
                    if (abs(king_c.y - c.y) == abs(king_c.x - c.x)) {
                        return true;
                    }
                    break;
                case Rook:
                    if (c.y == king_c.y) {
                        return true;
                    }
                    break;
                default:
                    std::cout << "Should not be reached under_attack_cords";
            }
        }
        return false;
    }
    
    int generate_pawn_moves(std::forward_list<Move>& moves, int x, int y, bool ignore_turns = false) {
        int moves_counter = 0;
        
        int direction_value, pawn_start_y, opposite_side_value;
        Cords orig = Cords {x, y};
        Move move = {orig, orig, Normal};
        
        // Switch some increment values depending on side
        if (squares[y][x].color == 1) {
            direction_value = 1;
            pawn_start_y = 1;
        }
        else {
            direction_value = -1;
            pawn_start_y = 6;
        }
        
        // Check for going straight ahead
        if (squares[y + direction_value][x].piece == Empty) {
            move.to_c.y += direction_value;
            moves_counter += pawn_path_handle_push_move(moves, move, ignore_turns);
            move.to_c = orig;
            // Check for double length for first pawn move
            if (y == pawn_start_y && squares[y + direction_value * 2][x].piece == Empty) {
                move.to_c.y += direction_value * 2;
                moves_counter += pawn_path_handle_push_move(moves, move, ignore_turns);
                move.to_c = orig;
            }
        }
        
        Cords king_c = current_turn == 1 ? black_king_loc : white_king_loc;
        int side_value = current_turn == 1 ? 4 : 3;

        bool need_to_calculate_pins = abs(en_passant_cords.x - king_c.x) == abs(side_value - king_c.y) || en_passant_cords.x == king_c.x || side_value == king_c.y;
         
        
        // See if Pawn can capture two diagonal squares
        for (int i = -1; i < 3; i+=2) {
            if (is_within_bounds(x + i, y + direction_value)) {
                if (squares[y + direction_value][x + i].piece != Empty) {
                    move.to_c.y += direction_value;
                    move.to_c.x += i;
                    moves_counter += pawn_path_handle_push_move(moves, move, ignore_turns);
                    move.to_c = orig;
                }
                // See if pawn can capture through en passant
                else if (en_passant_cords.y == y + direction_value && en_passant_cords.x == x + i) {
                    bool en_passant_is_pinned = false;
                    if (need_to_calculate_pins) {
                        // Check to see making an en_passant will result in exposing an attack on the king
                        int incr_x = sgn(en_passant_cords.x - king_c.x);
                        int incr_y = sgn(side_value - king_c.y);
                        en_passant_is_pinned = calculate_en_passant_pins(king_c, x, y, incr_x, incr_y);
                    }
                    if (!en_passant_is_pinned) {
                        move.to_c.y += direction_value;
                        move.to_c.x += i;
                        move.type = En_Passant;
                        moves_counter += pawn_path_handle_push_move(moves, move, ignore_turns);
                        move.to_c = orig;
                        move.type = Normal;
                    }
                }
            }
        }
        return moves_counter;
    }
    
    int pawn_path_handle_push_move(std::forward_list<Move>& moves, Move& move, bool ignore_turns = false) {
        if (!does_pass_basic_piece_checks(move, ignore_turns)) {
            return 0;
        }
        
        int opposite_side_value;
        if (squares[move.from_c.y][move.from_c.x].color == 1) {
            opposite_side_value = 7;
        }
        else {
            opposite_side_value = 0;
        }
        if (move.to_c.y == opposite_side_value) {
            move.type = Promote_to_Queen;
            moves.push_front(move);
            move.type = Promote_to_Rook;
            moves.push_front(move);
            move.type = Promote_to_Bishop;
            moves.push_front(move);
            move.type = Promote_to_Knight;
            moves.push_front(move);
            move.type = Normal;
            return 4;
        }
        else {
            moves.push_front(move);
            return 1;
        }
        return 0;
    }
    
    int reg_piece_handle_push_move(std::forward_list<Move>& moves, Move& move, bool ignore_turns = false) {
        if (!does_pass_basic_piece_checks(move, ignore_turns)) {
            return 0;
        }
        else {
            moves.push_front(move);
            return 1;
        }
    }
    
    int generate_king_moves(std::forward_list<Move>& moves, int x, int y, bool ignore_turns = false) {
        int moves_counter = 0;
        
        if (!(is_correct_turn(x, y)) && !ignore_turns) {
            return 0;
        }
        
        Cords orig = Cords {x, y};
        Move move = {orig, orig, Normal};
        
        int home_side_value = current_turn == 1 ? 0 : 7;
        
        for (int i = -1; i < 2; i++) {
            for (int j = -1; j < 2; j++) {
                if (i == 0 && j == 0) {
                    continue;
                }
                else if (!is_within_bounds(x + j, y + i)) {
                    continue;
                }
                else if (!is_square_under_attack(x + j, y + i, !current_turn)){
                    move.to_c = Cords{x + j, y + i};
                    moves_counter += reg_piece_handle_push_move(moves, move);
                    move.to_c = orig;
                }
            }
        }
        
        int can_castle_queenside = current_turn ? black_can_castle_queenside : white_can_castle_queenside;
        int can_castle_kingside = current_turn ? black_can_castle_kingside : white_can_castle_kingside;
        
        // Logic here -> yikes
        if (can_castle_queenside && squares[home_side_value][3].piece == Empty && squares[home_side_value][2].piece == Empty && squares[home_side_value][1].piece == Empty && !is_square_under_attack(move.from_c.x, move.from_c.y, !current_turn) && !is_square_under_attack(3, home_side_value, !current_turn) && !is_square_under_attack(2, home_side_value, !current_turn)) {
            move.to_c.x -= 2;
            move.type = Castle_Queenside;
            moves_counter += reg_piece_handle_push_move(moves, move);
            move.to_c = orig;
            move.type = Normal;
        }
        if (can_castle_kingside && squares[home_side_value][5].piece == Empty && squares[home_side_value][6].piece == Empty && !is_square_under_attack(move.from_c.x, move.from_c.y, !current_turn) && !is_square_under_attack(5, home_side_value, !current_turn) && !is_square_under_attack(6, home_side_value, !current_turn)) {
            move.to_c.x += 2;
            move.type = Castle_Kingside;
            moves_counter += reg_piece_handle_push_move(moves, move);
            move.to_c = orig;
            move.type = Normal;
        }
        
        return moves_counter;
    }
    
    int generate_knight_moves(std::forward_list<Move>& moves, int x, int y, bool ignore_turns = false) {
        int moves_counter = 0;
        
        if (!(is_correct_turn(x, y)) && !ignore_turns) {
            return;
        }
        
        Cords orig = Cords {x, y};
        Move move = {orig, orig, Normal};
        
        for (int i = -1; i < 2; i += 2) {
            for (int j = -1; j < 2; j += 2) {
                move.to_c.x += 1 * j;
                move.to_c.y += 2 * i;
                moves_counter += reg_piece_handle_push_move(moves, move);
                move.to_c = orig;
                
                move.to_c.x += 2 * j;
                move.to_c.y += 1 * i;
                moves_counter += reg_piece_handle_push_move(moves, move);
                move.to_c = orig;
            }
        }
        return moves_counter;
    }
    
    int generate_slider_moves(Cords* increments, Move& move, std::forward_list<Move>& moves, int size, int x, int y) {
        int moves_counter = 0;
        for (int i = 0; i < size; i++) {
            move.to_c = sliding_pieces_incrementer(x, y, (increments + i)->x, (increments + i)->y);
            int temp = std::max(abs(move.to_c.x - x), abs(move.to_c.y - y));
            for (int j = 0; j < temp; j++) {
                moves_counter += reg_piece_handle_push_move(moves, move);
                move.to_c.x -= (increments + i)->x;
                move.to_c.y -= (increments + i)->y;
            }
        }
        return moves_counter;
    }
    
    int generate_bishop_moves(std::forward_list<Move>& moves, int x, int y, bool ignore_turns = false) {
        int moves_counter = 0;
        
        if (!(is_correct_turn(x, y)) && !ignore_turns) {
            return 0;
        }
        
        Cords orig = Cords {x, y};
        Move move = {orig, orig, Normal};

        const int size = 4;
        Cords increments[size] = {Cords{-1, -1}, Cords{1, -1}, Cords{1, 1}, Cords{-1, 1}};
        
        moves_counter += generate_slider_moves(increments, move, moves, size, x, y);
        return moves_counter;
    }
    
    int generate_rook_moves(std::forward_list<Move>& moves, int x, int y, bool ignore_turns = false) {
        int moves_counter = 0;
        
        if (!(is_correct_turn(x, y)) && !ignore_turns) {
            return 0;
        }
        
        Cords orig = Cords {x, y};
        Move move = {orig, orig, Normal};

        const int size = 4;
        Cords increments[size] = {Cords{0, -1}, Cords{-1, 0}, Cords{0, 1}, Cords{1, 0}};
        
        moves_counter += generate_slider_moves(increments, move, moves, size, x, y);
        return moves_counter;
    }
    
    int generate_queen_moves(std::forward_list<Move>& moves, int x, int y, bool ignore_turns = false) {
        int moves_counter = 0;
        
        if (!(is_correct_turn(x, y)) && !ignore_turns) {
            return 0;
        }
        
        Cords orig = Cords {x, y};
        Move move = {orig, orig, Normal};

        const int size = 8;
        Cords increments[size] = {Cords{-1, -1}, Cords{1, -1}, Cords{1, 1}, Cords{-1, 1}, Cords{0, -1}, Cords{-1, 0}, Cords{0, 1}, Cords{1, 0}};
        
        moves_counter += generate_slider_moves(increments, move, moves, size, x, y);
        return moves_counter;
    }
    
    // TODO: Remove attacker_color maybe?
    std::forward_list<Cords> under_attack_cords(int x, int y, int attacker_color) {
        std::forward_list<Cords> attackers;
        
        // King check
        Cords enemy_king = current_turn == 1 ? white_king_loc : black_king_loc;
        int h1 = abs(enemy_king.x - x); int h2 = abs(enemy_king.y - y);
        if ((h1 <= 1 && h2 <= 1)  && !(h1 == 0 && h2 == 0)) {
            attackers.push_front(enemy_king);
        }

        
        // Pawn check
        int side_value;
        for (int i = -1; i < 2; i+=2) {
            side_value = current_turn == 1 ? 1 : -1;
            if (is_within_bounds(x+i, y+side_value)) {
                if (squares[y+side_value][x+i].piece == Pawn && squares[y+side_value][x+i].color != current_turn) {
                    attackers.push_front(Cords{x+i, y+side_value});
                }
            }
        }

        
        // Knight check
        for (int i = -1; i < 2; i += 2) {
            for (int j = -1; j < 2; j += 2) {
                if (is_within_bounds(x + 1 * j, y + 2 * i)) {
                    if (squares[y + 2 * i][x + 1 * j].piece == Knight && squares[y + 2 * i][x + 1 * j].color != current_turn) {
                        attackers.push_front(Cords{x + 1 * j, y + 2 * i});
                    }
                }
                if (is_within_bounds(x + 2 * j, y + 1 * i)) {
                    if (squares[y + 1 * i][x + 2 * j].piece == Knight && squares[y + 1 * i][x + 2 * j].color != current_turn) {
                        attackers.push_front(Cords{x + 2 * j, y + 1 * i});
                    }
                }
            }
        }

        // Slider pieces check
        Cords increments[8] = {Cords{-1, -1}, Cords{1, -1}, Cords{1, 1}, Cords{-1, 1}, Cords{0, -1}, Cords{-1, 0}, Cords{0, 1}, Cords{1, 0}};
        Cords c;
        
        for (int i = 0; i < 8; i++) {
            c = sliding_pieces_incrementer(x, y, increments[i].x, increments[i].y, true);
            if (c.x == x && c.y == y) {
                continue;
            }
            if (squares[c.y][c.x].color != current_turn) {
                switch (squares[c.y][c.x].piece) {
                    case Empty:
                    case Pawn:
                    case Knight:
                    case King:
                        break;
                    case Queen:
                        if (c.x == x || c.y == y || abs(y - c.y) == abs(x - c.x)) {
                            attackers.push_front(Cords{c.x, c.y});
                        }
                        break;
                    case Bishop:
                        if (abs(y - c.y) == abs(x - c.x)) {
                            attackers.push_front(Cords{c.x, c.y});
                        }
                        break;
                    case Rook:
                        if (c.x == x || c.y == y) {
                            attackers.push_front(Cords{c.x, c.y});
                        }
                        break;
                    default:
                        std::cout << "Should not be reached under_attack_cords";
                }
            }
        }

        /*
        std::cout << "Searching: " << x << ' ' << y << std::endl;
        for (auto it = attackers.begin(); it != attackers.end(); ++it) {
            std::cout << it->x << ' ' << it->y << std::endl;
        }
         */

        
        return attackers;
    }
    
    bool is_square_under_attack(int x, int y, int attacker_color) {
//        std::cout << x << ' ' << y << '|' << !under_attack_cords(x, y, attacker_color).empty() << '\n';
        return !under_attack_cords(x, y, attacker_color).empty();
    }
    
    bool is_friendly_piece(int x, int y) {
        return (squares[y][x].piece != Empty && squares[y][x].color == current_turn);
    }
    
    
    bool pawn_rules_subset(const Move &move, Move &validated_move) {
        int side_value, pawn_start_y;
        if (current_turn == 1) {
            side_value = 1;
            pawn_start_y = 1;
        }
        else {
            side_value = -1;
            pawn_start_y = 6;
        }
        
        if (move.to_c.y - move.from_c.y == side_value && move.to_c.x == move.from_c.x && squares[move.to_c.y][move.to_c.x].piece == Empty) {
            return true;
        }
        else if (abs(move.from_c.x - move.to_c.x) == 1 && move.to_c.y - move.from_c.y == side_value) {
            if (squares[move.to_c.y][move.to_c.x].piece != Empty) {
                return true;
            }
            else if (move.to_c.x == en_passant_cords.x && move.to_c.y == en_passant_cords.y) {
                validated_move.type = En_Passant;
                return true;
            }
        }
        else if (move.from_c.y - move.to_c.y == -2 * side_value && move.to_c.x - move.from_c.x == 0 && move.from_c.y == pawn_start_y && squares[move.to_c.y - side_value][move.to_c.x].piece == Empty) {
            return true;
        }
        return false;
    }
    
    bool is_pawn_move_valid(Move move, Move& validated_move) {
        int opposite_side_value;
        if (current_turn == 1) {
            opposite_side_value = 7;
        }
        else {
            opposite_side_value = 0;
        }
        if (move.type == Normal && move.to_c.y == opposite_side_value) {
            return false;
        }
        return pawn_rules_subset(move, validated_move);
    }
    
    bool is_king_move_valid(Move move, Move& validated_move) {
        int home_side_value = current_turn == 1 ? 0 : 7;
        int can_castle_queenside = current_turn ? black_can_castle_queenside : white_can_castle_queenside;
        int can_castle_kingside = current_turn ? black_can_castle_kingside : white_can_castle_kingside;
        
        if (is_square_under_attack(move.to_c.x, move.to_c.y, !current_turn)) {
            return false;
        }
        else if (abs(move.from_c.y - move.to_c.y) <= 1 && abs(move.from_c.x - move.to_c.x) <= 1) {
            return true;
        }
        else if (move.to_c.x == 2 && can_castle_queenside && !is_square_under_attack(3, home_side_value, !current_turn) && squares[home_side_value][3].piece == Empty && squares[home_side_value][2].piece == Empty && squares[home_side_value][1].piece == Empty && !is_square_under_attack(move.from_c.x, move.from_c.y, !current_turn)) {
            validated_move.type = Castle_Queenside;
            return true;
        }
        else if (move.to_c.x == 6 && can_castle_kingside && !is_square_under_attack(5, home_side_value, !current_turn) && squares[home_side_value][5].piece == Empty && squares[home_side_value][6].piece == Empty && !is_square_under_attack(move.from_c.x, move.from_c.y, !current_turn)) {
            validated_move.type = Castle_Kingside;
            return true;
        }
        
        return false;
    }
    
    bool is_knight_move_valid(int from_x, int from_y, int to_x, int to_y) {
        return ((abs(from_x - to_x) == 1 && (abs(from_y - to_y) == 2)) || (abs(from_x - to_x) == 2 && (abs(from_y - to_y) == 1)));
    }
    
    
    bool sliding_path_check(int from_x, int from_y, int to_x, int to_y) {
        Cords c = sliding_pieces_incrementer(from_x, from_y, sgn<int>(to_x-from_x), sgn<int>(to_y-from_y));
        return (abs(from_x - c.x) >= abs(from_x - to_x) && abs(from_y - c.y) >= abs(from_y - to_y));
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
    
    bool follows_pin_rules(Move move) {
        Cords king_c = current_turn == 1 ? black_king_loc : white_king_loc;
        
        // For some reason, pinned_by.find does work as expected. Cords initialization issues
        Cords& c = pinned_by[move.from_c];
//        std::cout << c.x << ' ' << c.y;
        if (c == Cords{-1, -1}) {
            pinned_by.erase(move.from_c);
            return true;
        }
        else {
            if (move.to_c == c) {
                return true;
            }
            else if (!is_in_between(king_c, c, move.to_c)) {
                return false;
            }
            switch(squares[c.y][c.x].piece) {
                case Queen:
                    if (c.x == move.to_c.x && king_c.x == move.to_c.x) {
                        return true;
                    }
                    else if (c.y == move.to_c.y && king_c.y == move.to_c.y) {
                        return true;
                    }
                    else if (abs(move.to_c.y - c.y) == abs(move.to_c.x - c.x)) {
                        return true;
                    }
                case Bishop:
                    if (abs(move.to_c.y - c.y) == abs(move.to_c.x - c.x)) {
                        return true;
                    }
                    break;
                case Rook:
                    if (c.x == move.to_c.x || c.y == move.to_c.y) {
                        return true;
                    }
                    break;
                default:
                    std::cout << "should not have been reached follows_pin_rules";
            }
        }
        return false;
    }
    
    bool is_following_piece_rules(Move move, Move& validated_move) {
        
        switch (squares[move.from_c.y][move.from_c.x].piece) {

            case Pawn:
                return is_pawn_move_valid(move, validated_move);
                break;
            case King:
                return is_king_move_valid(move, validated_move);
                break;
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
        
        return false;
    }
    
    Move is_move_valid(Move move) {
        Move validated_move = move;
        
        
        switch (validated_move.type) {
            case Promote_to_Queen:
            case Promote_to_Rook:
            case Promote_to_Bishop:
            case Promote_to_Knight:
                break;
            default:
                validated_move.type = Normal;
        }
        
        if (!does_pass_basic_piece_checks(move)) {
            validated_move.type = Illegal;
        }
        else if (!is_following_piece_rules(move, validated_move)) {
            validated_move.type = Illegal;
        }
        
    //    std::cout << x << " " << y << std::endl;
        return validated_move;
    }
    
    bool is_trying_to_promote(Move move) {
        int side_value = current_turn == 1 ? 7 : 0;
        
        // This is stupid but whatev
        Move holder_move = move;
        
        if (squares[move.from_c.y][move.from_c.x].piece != Pawn || !pawn_rules_subset(move, holder_move) || move.to_c.y != side_value) {
            return false;
        }
        else if (!does_pass_basic_piece_checks(move)) {
            return false;
        }
        return true;
    }
    
    void print_move(Move move, bool reg = false) {
        if (!reg) {
            std::cout << "From: " << move.from_c.x << ", " << move.from_c.y << " to: " << move.to_c.x << ", " << move.to_c.y << std::endl;
        }
        else {
            std::cout << num_to_char(move.from_c.x) << 8 - move.from_c.y << num_to_char(move.to_c.x) << 8 - move.to_c.y;
        }
    }
    
    void print_cords(Cords c) {
        std::cout << "Cords{" << c.x << ", " << c.y << '}';
    }
    
    
    void add_to_enemy_piece_values(int i) {
        if (current_turn) {
            white_piece_values += i;
        }
        else {
            black_piece_values += i;
        }
    }
    
    void add_to_home_piece_values(int i) {
        if (current_turn) {
            black_piece_values += i;
        }
        else {
            white_piece_values += i;
        }
    }
    
    // Legal moves must be generated before game end evals are called
    bool has_been_checkmated(const std::forward_list<Move>& moves) {
        /*
        std::cout << '\n' << '\n' << '\n' << '\n';
        for (auto it = legal_moves.begin(); it != legal_moves.end(); ++it) {
            print_move(*it);
        }
         */
        Cords king_c = current_turn == 1 ? black_king_loc : white_king_loc;
        return moves.empty() && is_square_under_attack(king_c.x, king_c.y, !current_turn);
    }
    
    bool is_draw(const std::forward_list<Move>& moves) {
        Cords king_c = current_turn == 1 ? black_king_loc : white_king_loc;
        if (moves.empty() && !is_square_under_attack(king_c.x, king_c.y, !current_turn)) {
            return true;
        }
        else if (previous_board_positions[remove_FEN_counters(generate_FEN())] == 2) {
            return true;
        }
        else {
            return false;
        }
    }
    
    void process_board_changes(const Move &move) {
        squares[move.to_c.y][move.to_c.x] = squares[move.from_c.y][move.from_c.x];
        Square square;
        square.piece = Empty;
        square.color = 0;
        squares[move.from_c.y][move.from_c.x] = square;
        
        // set some values for side-specific move patterns
        int castle_side_value, en_passant_side_value;
        if (current_turn == 1) {
            castle_side_value = 0;
            en_passant_side_value = 4;
        }
        else {
            castle_side_value = 7;
            en_passant_side_value = 3;
        }
        
        
        // Handle promotions, castling, en_passant
        switch (move.type) {
            case Promote_to_Queen:
                squares[move.to_c.y][move.to_c.x].piece = Queen;
                add_to_home_piece_values(QUEEN_VALUE - PAWN_VALUE);
                break;
            case Promote_to_Rook:
                squares[move.to_c.y][move.to_c.x].piece = Rook;
                add_to_home_piece_values(ROOK_VALUE - PAWN_VALUE);
                break;
            case Promote_to_Bishop:
                squares[move.to_c.y][move.to_c.x].piece = Bishop;
                add_to_home_piece_values(BISHOP_VALUE - PAWN_VALUE);
                break;
            case Promote_to_Knight:
                squares[move.to_c.y][move.to_c.x].piece = Knight;
                add_to_home_piece_values(KNIGHT_VALUE - PAWN_VALUE);
                break;
            case Castle_Queenside:
                squares[castle_side_value][3] = squares[castle_side_value][0];
                squares[castle_side_value][0].piece = Empty;
                squares[castle_side_value][0].color = 0;
                break;
            case Castle_Kingside:
                squares[castle_side_value][5] = squares[castle_side_value][7];
                squares[castle_side_value][7].piece = Empty;
                squares[castle_side_value][7].color = 0;
                break;
            case En_Passant:
                squares[en_passant_side_value][move.to_c.x].piece = Empty;
                add_to_enemy_piece_values(-PAWN_VALUE);
                break;
            case Normal:
                break;
            case Illegal:
            default:
                std::cout << "This should not have been reached (process_board_changes).";
                break;
        }
    }
    
    std::string remove_FEN_counters(std::string in_str) {
        std::string str;
        
        bool flag = false;
        int pos;

        for (pos = in_str.length() - 1; pos >= 0; pos--) {
            if (in_str[pos] == ' ') {
                if (!flag) {
                    flag = true;
                }
                else {
                    break;
                }
            }
//            std::cout << pos << std::endl;
        }
        str = in_str.substr(0, pos);
//        std::cout << str;
        return str;
    }
    
    void clear_attacks_on_king() {
        attacks_on_the_king.clear();
    }
    
    void process_move(Move move) {
        
        // Save move info onto past moves stack
        Move_data move_data;
        move_data.move = move;
        move_data.captured_piece = squares[move.to_c.y][move.to_c.x].piece;
        move_data.previous_en_passant_cords = en_passant_cords;
        move_data.white_can_castle_kingside = white_can_castle_kingside;
        move_data.white_can_castle_queenside = white_can_castle_queenside;
        move_data.black_can_castle_kingside = black_can_castle_kingside;
        move_data.black_can_castle_queenside = black_can_castle_queenside;
        
        move_stack.push_front(move_data);
        
        

        // If square was captured, change the enemy piece values accordingly
        add_to_enemy_piece_values(-piece_to_value(squares[move.to_c.y][move.to_c.x].piece));
        
        
        // Add the current board position for 3-move repition check
        
//        previous_board_positions[remove_FEN_counters(generate_FEN())] += 1;
        

        // Check to see if castling is invalidated
        
        // see if rook was captured/moved
        int eval_x = move.to_c.x;
        int eval_y = move.to_c.y;
        for (int i = 0; i < 2; i++) {
            if (eval_x == 0 && eval_y == 0) {
                black_can_castle_queenside = false;
            }
            else if (eval_x == 7 && eval_y == 0) {
                black_can_castle_kingside = false;
            }
            else if (eval_x == 0 && eval_y == 7) {
                white_can_castle_queenside = false;
            }
            else if (eval_x == 7 && eval_y == 7) {
                white_can_castle_kingside = false;
            }
            
            eval_x = move.from_c.x;
            eval_y = move.from_c.y;
        }
        
        // Check if king moves
        if (squares[move.from_c.y][move.from_c.x].piece == King) {
            if (current_turn == 1) {
                black_can_castle_kingside = false;
                black_can_castle_queenside = false;
            }
            else {
                white_can_castle_kingside = false;
                white_can_castle_queenside = false;
            }
        }
        
        
        
        
        // TODO: use ternary ops instead of if-else
        
        // Check if En Passant cords need to be set
        if (squares[move.from_c.y][move.from_c.x].piece == Pawn && abs(move.from_c.y - move.to_c.y) == 2) {
            if (current_turn == 1) {
                en_passant_cords.x = move.to_c.x;
                en_passant_cords.y = move.to_c.y - 1;
            }
            else {
                en_passant_cords.x = move.to_c.x;
                en_passant_cords.y = move.to_c.y + 1;
            }
        }
        else {
            // Clear En Passant
            en_passant_cords.x = -1;
            en_passant_cords.y = -1;
        }
        
        // If moved to/from squares in pin paths, delete old pins, prep recalculate pins
        // The reason this is done pre-actual move is because delete_pins *must* be called before the board is changed.
        Cords king_c;
        int generate_pins_info[2] = {0, 0};
        int increments_white[8] = {0};
        int increments_black[8] = {0};
        int* increments;
        king_c = current_turn == 1 ? black_king_loc : white_king_loc;

        if (!(move.type == Castle_Kingside || move.type == Castle_Queenside || move.type == En_Passant || move.from_c == king_c)) {
            for (int i = 0; i < 2; i++) {
                
                king_c = i == 1 ? black_king_loc : white_king_loc;
                increments = i == 1 ? increments_black : increments_white;
                

                if (abs(move.from_c.x - king_c.x) == abs(move.from_c.y - king_c.y) || move.from_c.x == king_c.x || move.from_c.y == king_c.y) {
                    *(increments + increment_to_index(Cords{sgn(move.from_c.x - king_c.x), sgn(move.from_c.y - king_c.y)})) = 1;
                    generate_pins_info[i] = 1;
                }
                
                if (abs(move.to_c.x - king_c.x) == abs(move.to_c.y - king_c.y) || move.to_c.x == king_c.x || move.to_c.y == king_c.y) {
                    *(increments + increment_to_index(Cords{sgn(move.to_c.x - king_c.x), sgn(move.to_c.y - king_c.y)})) = 1;
                    generate_pins_info[i] = 1;
                }
                
                if (generate_pins_info[i]) {
                    delete_pins(i, increments);
                }
            }
        } else {
            delete_pins(0, incre8);
            delete_pins(1, incre8);
            generate_pins_info[0] = 2;
            generate_pins_info[1] = 2;
        }
        
        if (squares[move.from_c.y][move.from_c.x].piece == King) {
            if (current_turn == 1) {
                black_king_loc = move.to_c;
            }
            else {
                white_king_loc = move.to_c;
            }
        }
        
        // Actual act of moving pieces
        process_board_changes(move);
        
        
        
        // Finish pins
        for (int i = 0; i < 2; i++) {
            increments = i == 1 ? increments_black : increments_white;
            if (generate_pins_info[i] == 1) {
                
                /*
                for (int x = 0; x < 8; x++) {
                    std::cout << *(increments + x) << ' ';
                }
                std::cout << '\n';
                 */
                generate_pins(i, increments);
            }
            else if (generate_pins_info[i] == 2) {
                generate_pins(i, incre8);
            }
        }
        
        /*
        debug_attacked_squares(current_turn);
        auto copy1 = attacking;
        auto copy2 = attacked_by_black;
        auto copy3 = attacked_by_white;
        generate_attacked_squares();
        debug_attacked_squares(current_turn);
        
        attacking = copy1;
        attacked_by_black = copy2;
        attacked_by_white = copy3;
        */

        current_turn = !current_turn;

        /*
        generate_moves(legal_moves);
        
        if (has_been_checkmated()) {
            std::cout << "Checkmate!" << std::endl;
        }
        if (is_draw()) {
            std::cout << "Draw has been reached. " << std::endl;
        }
         */
        
//        debug_print();
//        undo_last_move();
//        debug_print();
//        std::cout << generate_FEN();
    }
    
    void undo_last_move() {
        Move_data move_data = *(move_stack.begin());
//        previous_board_positions[remove_FEN_counters(generate_FEN())] -= 1;
        
        current_turn = !current_turn;
        
        Cords king_c;
        int generate_pins_info[2] = {0, 0};
        int increments_white[8] = {0};
        int increments_black[8] = {0};
        int* increments;
        king_c = current_turn == 1 ? black_king_loc : white_king_loc;

        if (!(move_data.move.type == Castle_Kingside || move_data.move.type == Castle_Queenside || move_data.move.type == En_Passant || move_data.move.to_c == king_c)) {
            for (int i = 0; i < 2; i++) {
                
                king_c = i == 1 ? black_king_loc : white_king_loc;
                increments = i == 1 ? increments_black : increments_white;
                
                if (abs(move_data.move.from_c.x - king_c.x) == abs(move_data.move.from_c.y - king_c.y) || move_data.move.from_c.x == king_c.x || move_data.move.from_c.y == king_c.y) {
                    *(increments + increment_to_index(Cords{sgn(move_data.move.from_c.x - king_c.x), sgn(move_data.move.from_c.y - king_c.y)})) = 1;
                    generate_pins_info[i] = 1;
                }
                
                if (abs(move_data.move.to_c.x - king_c.x) == abs(move_data.move.to_c.y - king_c.y) || move_data.move.to_c.x == king_c.x || move_data.move.to_c.y == king_c.y) {
                    *(increments + increment_to_index(Cords{sgn(move_data.move.to_c.x - king_c.x), sgn(move_data.move.to_c.y - king_c.y)})) = 1;
                    generate_pins_info[i] = 1;
                }
                
                if (generate_pins_info[i]) {
                    delete_pins(i, increments);
                }
            }
        } else {
            delete_pins(0, incre8);
            delete_pins(1, incre8);
            generate_pins_info[0] = 2;
            generate_pins_info[1] = 2;
        }
        
        
        
        if (squares[move_data.move.to_c.y][move_data.move.to_c.x].piece == King) {
            if (current_turn == 1) {
            // Change the king_loc as well
                black_king_loc = move_data.move.from_c;
            }
            else {
                white_king_loc = move_data.move.from_c;
            }
        }
        
        
        
        squares[move_data.move.from_c.y][move_data.move.from_c.x] = squares[move_data.move.to_c.y][move_data.move.to_c.x];
        squares[move_data.move.to_c.y][move_data.move.to_c.x].piece = move_data.captured_piece;
        squares[move_data.move.to_c.y][move_data.move.to_c.x].color = !current_turn;
        
        // Take the captured piece, and add its value back to enemy piece_values
        add_to_enemy_piece_values(piece_to_value(move_data.captured_piece));
        
        // set some values for side-specific move patterns
        int castle_side_value, en_passant_side_value;
        if (current_turn == 1) {
            castle_side_value = 0;
            en_passant_side_value = 4;
        }
        else {
            castle_side_value = 7;
            en_passant_side_value = 3;
        }
        
        
        // Handle promotions, castling, en_passant
        switch (move_data.move.type) {
            case Promote_to_Queen:
                // return the piece_values back to what they were before promotion
                add_to_home_piece_values(PAWN_VALUE - QUEEN_VALUE);
                squares[move_data.move.from_c.y][move_data.move.from_c.x].piece = Pawn;
                break;
            case Promote_to_Rook:
                add_to_home_piece_values(PAWN_VALUE - ROOK_VALUE);
                squares[move_data.move.from_c.y][move_data.move.from_c.x].piece = Pawn;
                break;
            case Promote_to_Bishop:
                add_to_home_piece_values(PAWN_VALUE - BISHOP_VALUE);
                squares[move_data.move.from_c.y][move_data.move.from_c.x].piece = Pawn;
                break;
            case Promote_to_Knight:
                add_to_home_piece_values(PAWN_VALUE - KNIGHT_VALUE);
                squares[move_data.move.from_c.y][move_data.move.from_c.x].piece = Pawn;
                break;
            case Castle_Queenside:
                squares[castle_side_value][0] = squares[castle_side_value][3];
                squares[castle_side_value][3].piece = Empty;
                squares[castle_side_value][3].color = 0;
                break;
            case Castle_Kingside:
                squares[castle_side_value][7] = squares[castle_side_value][5];
                squares[castle_side_value][5].piece = Empty;
                squares[castle_side_value][5].color = 0;
                break;
            case En_Passant:
                squares[en_passant_side_value][move_data.move.to_c.x].piece = Pawn;
                squares[en_passant_side_value][move_data.move.to_c.x].color = !current_turn;
                add_to_enemy_piece_values(PAWN_VALUE);
                break;
            case Normal:
                break;
            case Illegal:
            default:
                std::cout << "This should not have been reached (process_move).";
                break;
        }
        
        
        for (int i = 0; i < 2; i++) {
            increments = i == 1 ? increments_black : increments_white;
            if (generate_pins_info[i] == 1) {
                generate_pins(i, increments);
            }
            else if (generate_pins_info[i] == 2) {
                generate_pins(i, incre8);
            }
        }
        
        en_passant_cords = move_data.previous_en_passant_cords;
        white_can_castle_kingside = move_data.white_can_castle_kingside;
        white_can_castle_queenside = move_data.white_can_castle_queenside;
        black_can_castle_kingside = move_data.black_can_castle_kingside;
        black_can_castle_queenside = move_data.black_can_castle_queenside;
        
        move_stack.pop_front();
    }
    
    void debug_piece_values() {
        std::cout << "white_piece_values: " << white_piece_values << " | black_piece_values: " << black_piece_values << '\n';
    }
    
    long Perft(int depth /* assuming >= 1 */) {
        long nodes = 0;
        int n_moves = 0;

        std::forward_list<Move> moves;
        n_moves = generate_moves(moves);
        
        if (depth == 0) {
            return 1;
        }
        
        /*
        if (depth == 1) {
            return n_moves;
        }
         */

        for (auto it = moves.begin(); it != moves.end(); ++it) {
            process_move(*it);
            nodes += Perft(depth - 1);
            undo_last_move();
        }
        return nodes;
    }
    
    void sort_moves(std::forward_list<Move>& moves) {
        int score;
        
        // Score all the moves
        for (auto it = moves.begin(); it != moves.end(); ++it) {
            score = 0;
            if (squares[it->to_c.y][it->to_c.x].piece != Empty) {
                score += 50;
                score += piece_to_value(squares[it->to_c.y][it->to_c.x].piece) - piece_to_value(squares[it->from_c.y][it->from_c.x].piece);
            }
            if (it->type == Promote_to_Queen || it->type == Promote_to_Rook || it->type == Promote_to_Bishop || it->type == Promote_to_Knight) {
                score += 100;
            }
            else if (it->type == Castle_Kingside || it->type == Castle_Queenside) {
                score += 60;
            }
            it->score = score;
        }
        
        moves.sort(move_cmp);
    }
    
    
    int static_eval(/*std::forward_list<Move>& moves*/) {
        int eval = 0;
        
        eval += white_piece_values - black_piece_values;
        
        
        eval *= current_turn ? -1 : 1;
        return eval;
    }
    
    int negamax(int depth, int alpha, int beta) {
        std::forward_list<Move> moves;
        generate_moves(moves);
        sort_moves(moves);
        
        
        if (has_been_checkmated(moves)) {
            return current_turn == 0 ? 2000000 : -2000000;
        }
        else if (is_draw(moves)) {
            return 0;
        }
        else if (depth == 0) {
            return static_eval();
        }
        
        


        for (auto it = moves.begin(); it != moves.end(); ++it) {
            process_move(*it);
            int eval = -negamax(depth - 1, -beta, -alpha);
            undo_last_move();
            if (eval > beta) {
                return beta;
            }
            alpha = std::max(alpha, eval);
        }
        
        return alpha;
    }
    
    Move find_best_move(int depth) {
        std::forward_list<Move> moves;
        generate_moves(moves);
        
        Move best_move;
        int maxEval = -2000000;
        
        for (auto it = moves.begin(); it != moves.end(); ++it) {
            process_move(*it);
            int eval = -negamax(depth - 1, -2000000, 2000000);
            if (eval > maxEval) {
                maxEval = eval;
                best_move = *it;
            }
            undo_last_move();
        }
        
        return best_move;
    }
    
    Move request_move(Move move){
        // This functions takes in a move requested by the board, and returns the correct type of it is,
        // whilst updating the internal board appropriately. It checks if the move is illegal, and whether it is a special move
        Move validated_move = is_move_valid(move);
        if (validated_move.type != Illegal) {
            process_move(validated_move);
        }
        return validated_move;
    }
};



void draw_pieces(sf::RenderWindow* window) {
    for (std::forward_list<sf::Sprite>::iterator it = sprites.begin() ; it != sprites.end(); ++it) {
        window->draw(*it);
    }
}


void draw_promotion_pieces(sf::RenderWindow* window, int current_turn) {
    if (current_turn) {
        for (std::forward_list<sf::Sprite>::iterator it = promotion_sprites_black.begin() ; it != sprites.end(); ++it) {
            window->draw(*it);
        }
    }
    else {
        for (std::forward_list<sf::Sprite>::iterator it = promotion_sprites_white.begin() ; it != sprites.end(); ++it) {
            window->draw(*it);
        }
    }
}



sf::Sprite* locate_sprite_clicked(std::forward_list<sf::Sprite>& list, int x, int y) {
    for (std::forward_list<sf::Sprite>::iterator it = list.begin() ; it != list.end(); ++it) {
        if ((*it).getGlobalBounds().contains(x, y)) {
            return (&(*it));
        }
    }
    return NULL;
}



int locate_sprite_clicked_index(std::forward_list<sf::Sprite>& list, int x, int y, sf::Sprite* sprite) {
    int counter = 0;
    for (std::forward_list<sf::Sprite>::iterator it = list.begin() ; it != list.end(); ++it) {
        if ((*it).getGlobalBounds().contains(x, y) && (&(*it) != sprite)) {
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

void set_single_promotion_texture(int color, int i, sf::Sprite& sprite) {
    piece_type piece;
    switch (i) {
        case 0:
            piece = Queen;
            break;
        case 1:
            piece = Rook;
            break;
        case 2:
            piece = Bishop;
            break;
        case 3:
            piece = Knight;
            break;
        default:
            std::cout << "Should not have been reached. " << std::endl;
            break;
    }
    set_single_texture(color, piece, sprite);
}

void set_promotional_sprites() {
    for (int c = 0; c < 2; c++) {
        for (int i = 0; i < 4; i++) {
            sf::Sprite sprite;
            set_single_promotion_texture(c, i, sprite);
            
            sprite.setOrigin(sf::Vector2f(30, 30));
            sprite.setPosition(i * WIDTH/8 + WIDTH/16 + WIDTH / 4, WIDTH / 2);
            sprite.setScale(sf::Vector2f(SCALE, SCALE));
            
            if (c == 0) {
                promotion_sprites_black.push_front(sprite);
            }
            else {
                promotion_sprites_white.push_front(sprite);
            }
        }
    }
}


void castle_sprite_handler(int castle_side_value, Move validated_move) {
    if (validated_move.type == Castle_Kingside) {
        sf::Sprite* temp_sprite = locate_sprite_clicked(sprites, 7 * WIDTH/8 + WIDTH/16 - OFFSET, castle_side_value * WIDTH/8 + WIDTH/16 - OFFSET);
        temp_sprite->setPosition(5 * WIDTH/8 + WIDTH/16 - OFFSET, castle_side_value * WIDTH/8 + WIDTH/16 - OFFSET);
    }
    else if (validated_move.type == Castle_Queenside) {
        sf::Sprite* temp_sprite = locate_sprite_clicked(sprites, 0 * WIDTH/8 + WIDTH/16 - OFFSET, castle_side_value * WIDTH/8 + WIDTH/16 - OFFSET);
        temp_sprite->setPosition(3 * WIDTH/8 + WIDTH/16 - OFFSET, castle_side_value * WIDTH/8 + WIDTH/16 - OFFSET);
    }
}



void en_passant_sprite_handler(int en_passant_side_value, Move move, Move validated_move) {
    int temp_index;
    if (validated_move.type == En_Passant) {
        temp_index = locate_sprite_clicked_index(sprites, move.to_c.x * WIDTH/8 + WIDTH/16 - OFFSET, en_passant_side_value * WIDTH/8 + WIDTH/16 - OFFSET, NULL);
        if (temp_index != -1) {
            std::forward_list<sf::Sprite>::iterator en_p_it = sprites.before_begin();
            std::advance(en_p_it, temp_index);
            sprites.erase_after(en_p_it);
        }
    }
}


void normal_move_sprite_handler(Move validated_move, sf::Sprite* sprite_being_dragged) {
    int temp_index = locate_sprite_clicked_index(sprites, validated_move.to_c.x * WIDTH/8 + WIDTH/16 - OFFSET, validated_move.to_c.y * WIDTH/8 + WIDTH/16 - OFFSET, sprite_being_dragged);
    
//    std::cout << c.x << ' ' << c.y << std::endl;
    sprite_being_dragged->setPosition(validated_move.to_c.x * WIDTH/8 + WIDTH/16 - OFFSET, validated_move.to_c.y * WIDTH/8 + WIDTH/16 - OFFSET);
    if (temp_index != -1) {
        std::forward_list<sf::Sprite>::iterator it = sprites.before_begin();
        std::advance(it, temp_index);
        sprites.erase_after(it);
    }
}


int main() {
    
    /*
    google::dense_hash_map<Cords, Cords, std::hash<Cords>, cords_eq> a;
    a.set_empty_key(Cords{-1, -1});
    a[Cords{1, 1}] = Cords{1, 1};
    std::cout << a[Cords{1, 2}].x;
    */
    
    for (int i = 0; i < 8; i++) {
        incre8[i] = 1;
    }
    
    
    
    
    sf::Sprite* sprite_being_dragged;
    sprite_being_dragged = NULL;
    
    Move move, validated_move;
    bool trying_to_promote = false;

    // create the window
    sf::RenderWindow window(sf::VideoMode(WIDTH, WIDTH), "My window");
    window.setFramerateLimit(60);
    
    load_textures();
    set_promotional_sprites();
    
    
    sf::RectangleShape displaygrid[8][8];
    // create the Grid
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // init rectangle
            sf::RectangleShape tempRect(sf::Vector2f(WIDTH/8, WIDTH/8));
            
            // Make checkerboard pattern
            if ((x + y) % 2 == 0) {
                tempRect.setFillColor(sf::Color(143, 101, 83));
            }
            else {
                tempRect.setFillColor(sf::Color(84, 54, 41));
            }
            
            // move to proper location and draw
            tempRect.setPosition(x * WIDTH/8, y * WIDTH/8);
            displaygrid[y][x] = tempRect;
        }
    }
    
    // Create the promotion rectangle
    sf::RectangleShape promotion_rectangle(sf::Vector2f(WIDTH/2, WIDTH/8));
    promotion_rectangle.setFillColor(sf::Color(26, 110, 8, 200));
    promotion_rectangle.setPosition(WIDTH / 4, WIDTH / 2 - WIDTH / 16);
    
    // init the chess board
    Board board("r3k2r/p1ppqpb1/Bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 0");

    std::forward_list<Move> moves;
    board.generate_moves(moves);
    
    
    clock_t begin = clock();
    /*
    int counter = 0;
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        board.print_move(*it, true);
        board.process_move(*it);
//        counter += board.Perft(5);
        std::cout << ": " << board.Perft(1) << std::endl;
//        board.debug_print();
//        board.debug_pins();
        board.undo_last_move();
    }
     */

    
//    std::cout << board.Perft(3) << std::endl;
//    std::cout<<counter;
    board.print_move(board.find_best_move(4), true);
    std::cout << '\n';

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    
    std::cout << "Time: " << elapsed_secs << "s\n";
    
    board.clear_attacks_on_king();
    board.set_texture_to_pieces();
    
    
    /*
    // Measuring FPS
    sf::Clock clock;
    int counter = 0;
    */
    
    
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
                if (event.mouseButton.button == sf::Mouse::Left && !sprite_being_dragged)
                {
                    if (trying_to_promote) {
                        int temp_index = locate_sprite_clicked_index(promotion_sprites_black, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y, NULL);
                        if (temp_index != -1) {
                            switch (temp_index) {
                                case 3:
                                    move.type = Promote_to_Queen;
                                    break;
                                case 2:
                                    move.type = Promote_to_Rook;
                                    break;
                                case 1:
                                    move.type = Promote_to_Bishop;
                                    break;
                                case 0:
                                    move.type = Promote_to_Knight;
                                    break;
                                default:
                                    std::cout << "Something went wrong about promotion. " << std::endl;
                                    break;
                            }

                            validated_move = board.request_move(move);
//                            board.debug_print();
                            
                            sf::Sprite* sprite = locate_sprite_clicked(sprites, validated_move.from_c.x * WIDTH/8 + WIDTH/16 - OFFSET, validated_move.from_c.y * WIDTH/8 + WIDTH/16 - OFFSET);
                            
                            set_single_promotion_texture(!board.get_current_turn(), 3 - temp_index, *sprite);
                            normal_move_sprite_handler(validated_move, sprite);
                            
                            trying_to_promote = false;
                        }
                    }
                    else {
                        // save the sprite being dragged
                        sprite_being_dragged = locate_sprite_clicked(sprites, sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                        if (sprite_being_dragged) {
                            move.from_c = find_grid_bounds(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                        }
                    }
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (sprite_being_dragged) {
    
                        move.to_c = find_grid_bounds(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
                        move.type = Normal;
                        trying_to_promote = board.is_trying_to_promote(move);
                        
                        
                        if (!trying_to_promote) {
                            
                            int castle_side_value, en_passant_side_value;
                            if (board.get_current_turn() == 1) {
                                en_passant_side_value = 4;
                                castle_side_value = 0;
                            }
                            else {
                                en_passant_side_value = 3;
                                castle_side_value = 7;
                            }
                            
                            
                            validated_move = board.request_move(move);
                            
                            // This section checks and handles the validity of move, including drawing the sprite
                            if (validated_move.type != Illegal) {
                                std::cout << board.generate_FEN() << '\n';
    
                                // If move is valid, set the sprite to the new position, delete the sprite that was residing in the to_location, and register the move with the board.
    
    
                                // Standard move (Move piece from cords A to cords B, delete the sprite that is currently at the target location)
                                normal_move_sprite_handler(validated_move, sprite_being_dragged);
                                
                                switch (validated_move.type) {
                                    case Castle_Kingside:
                                    case Castle_Queenside:
                                        // Castle handler
                                        castle_sprite_handler(castle_side_value, validated_move);
                                        break;
                                    case En_Passant:
                                        // En Passant Handler
                                        en_passant_sprite_handler(en_passant_side_value, move, validated_move);
                                        break;
                                    default:
                                        break;
                                }

                                
//                                board.debug_print();
                            }
                            else {
                                // If move isn't valid, return sprite to original position and do nothing
                                sprite_being_dragged->setPosition(move.from_c.x * WIDTH/8 + WIDTH/16 - OFFSET, move.from_c.y * WIDTH/8 + WIDTH/16 - OFFSET);
                            }
                        }
                        else {
                            // If move is trying to promote, return sprite to original position and do nothing
                            sprite_being_dragged->setPosition(move.from_c.x * WIDTH/8 + WIDTH/16 - OFFSET, move.from_c.y * WIDTH/8 + WIDTH/16 - OFFSET);
                        }
                        
                        sprite_being_dragged = NULL;
                    }
                }
            }
        }

        
        // Do Calcs/Pos changes
        
        if (sprite_being_dragged) {
            sprite_being_dragged->setPosition(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y);
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
        

        if (trying_to_promote) {
            window.draw(promotion_rectangle);
            draw_promotion_pieces(&window, !board.get_current_turn());
        }
        
        
        
        
        // end the current frame
        window.display();
        
        
        /*
        // Measuring FPS
        counter++;
        if (counter == 5000) {
            std::cout << (5000.0 / clock.restart().asSeconds()) << std::endl;
            counter = 0;
        }
        */
    }

    return EXIT_SUCCESS;
}
