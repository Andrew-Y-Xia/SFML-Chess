//
//  Board.hpp
//  SFML Chess
//
//  Created by Andrew Xia on 4/16/21.
//  Copyright © 2021 Andy. All rights reserved.
//

#ifndef Board_hpp
#define Board_hpp

#include "depend.hpp"
#include "Board.hpp"
#include "Data_structs.hpp"
#include "Utility.hpp"

void set_single_texture(int color, piece_type piece, sf::Sprite& sprite);

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
    Board();
    
    void find_kings();
    
    void standard_setup();
    
    void post_readLEN();
    
    Board(std::string str);
    
    int get_current_turn();
    
    int piece_to_value(piece_type piece);
    
    void reset_piece_values();
    
    void set_texture_to_pieces();
    
    
    std::string generate_FEN();
    
    
    void read_FEN(std::string str);
    
    char piece_type_to_char(piece_type p);
    
    void debug_print();
    
    Cords sliding_pieces_incrementer(int x, int y, int increment_x, int increment_y, bool ignore_king = false);
    
    Cords ignore_square_incrementer(int x, int y, int increment_x, int increment_y, Cords ignore_squares[2]);
    
    void debug_attacked_squares(int attacker_color);
    
    // Lots of boilerplate here
    void pin_slider(int x, int y, int increment_x, int increment_y, Cords& pinned_piece, Cords& pinning_piece);
    
    void debug_pins();
    
    
    void delete_pins(int color, int* increments);
    
    void generate_pins(int color, int* increments);
    
    bool is_in_between(Cords c1, Cords c2, Cords move_to);
    
    void print_attacks_on_king();


    bool follows_check_rules(Move move);
    
    bool does_pass_basic_piece_checks(Move move);
    
    void reset_attacks_on_the_king();
    
    int generate_moves(std::vector<Move>& moves);
    
    bool calculate_en_passant_pins(const Cords &king_c, int x, int y, int incr_x, int incr_y);
    
    int generate_pawn_moves(std::vector<Move>& moves, int x, int y, bool ignore_turns = false);
    
    int pawn_path_handle_push_move(std::vector<Move>& moves, Move& move, bool ignore_turns = false);
    
    int reg_piece_handle_push_move(std::vector<Move>& moves, Move& move);
    
    int generate_king_moves(std::vector<Move>& moves, int x, int y, bool ignore_turns = false);
    
    int generate_knight_moves(std::vector<Move>& moves, int x, int y, bool ignore_turns = false);
    
    int generate_slider_moves(Cords* increments, Move& move, std::vector<Move>& moves, int size, int x, int y);
    
    int generate_bishop_moves(std::vector<Move>& moves, int x, int y, bool ignore_turns = false);
    
    int generate_rook_moves(std::vector<Move>& moves, int x, int y, bool ignore_turns = false);
    
    int generate_queen_moves(std::vector<Move>& moves, int x, int y, bool ignore_turns = false);
    
    // TODO: Remove attacker_color maybe?
    std::forward_list<Cords> under_attack_cords(int x, int y, int attacker_color);
    
    bool is_square_under_attack(int x, int y, int attacker_color);
    
    bool is_friendly_piece(int x, int y);
    
    
    bool pawn_rules_subset(const Move &move, Move &validated_move);
    
    bool is_pawn_move_valid(Move move, Move& validated_move);
    
    bool is_king_move_valid(Move move, Move& validated_move);
    
    bool is_knight_move_valid(int from_x, int from_y, int to_x, int to_y);
    
    
    bool sliding_path_check(int from_x, int from_y, int to_x, int to_y);
    
    bool is_bishop_move_valid(int from_x, int from_y, int to_x, int to_y);
    
    bool is_rook_move_valid(int from_x, int from_y, int to_x, int to_y);
    
    bool is_queen_move_valid(int from_x, int from_y, int to_x, int to_y);
    
    bool is_correct_turn(int x, int y);
    
    bool is_within_bounds(int x, int y);
    
    bool follows_pin_rules(Move move);
    
    bool is_following_piece_rules(Move move, Move& validated_move);
    
    Move is_move_valid(Move move);
    
    bool is_trying_to_promote(Move move);
    
    void add_to_enemy_piece_values(int i);
    
    void add_to_home_piece_values(int i);
    
    // Legal moves must be generated before game end evals are called
    bool has_been_checkmated(const std::vector<Move>& moves);
    
    bool is_draw(const std::vector<Move>& moves);
    
    void process_board_changes(const Move &move);
    
    std::string remove_FEN_counters(std::string in_str);
    
    void clear_attacks_on_king();
    
    void process_move(Move move);
    
    void undo_last_move();
    
    void debug_piece_values();
    
    long Perft(int depth /* assuming >= 1 */);
    
    void sort_moves(std::vector<Move>& moves);
    
    
    int static_eval(/*std::forward_list<Move>& moves*/);
    
    int negamax(int depth, int alpha, int beta);
    
    int quiescence_search(int alpha, int beta);
    
    Move request_move(Move move);
};

#endif /* Board_hpp */
