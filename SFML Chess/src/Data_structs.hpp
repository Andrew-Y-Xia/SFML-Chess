//
//  Data_structs.hpp
//  SFML Chess
//
//  Created by Andrew Xia on 4/16/21.
//  Copyright © 2021 Andy. All rights reserved.
//

#ifndef Data_structs_hpp
#define Data_structs_hpp

#include <iostream>
#include <string>

struct Cords {
    int x: 8;
    int y: 8;
    
    bool operator==(const Cords c2);
    bool operator!=(const Cords c2);
    Cords();
    Cords(int a, int b);
};


namespace std {
    template<>
    struct hash<Cords>
    {
        size_t operator()(Cords const& c) const noexcept;
    };
}

struct cords_eq {
    bool operator()(Cords c1, Cords c2) const;
};

struct eqstr
{
    bool operator()(std::string s1, std::string s2) const;
};

template <class T, class Sub>
bool lookup(T Set, Sub word);

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

bool move_cmp(Move first, Move second);

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



#endif /* Data_structs_hpp */
