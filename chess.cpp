// =====================================================================
//  CONSOLE CHESS  -  Data Structures & Algorithms Group Project
//
//  Data structures and algorithms used in this program:
//   1. 2D Array      -> the 8x8 chess board
//   2. Stack         -> move history (used by "undo")
//   3. Queue         -> turn order between the two players (round robin)
//   4. Linked List   -> captured pieces of each player
//   5. Linear Search -> find the king on the board (for "Check!")
//   6. Bubble Sort   -> sort captured pieces by their value
//
//  Compile:  g++ chess.cpp -o chess
// =====================================================================
#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

const int SIZE = 8;          // the board is 8 x 8
const int MAX_MOVES = 1000;  // maximum moves the history stack can hold
const int WHITE = 0;         // player number for White
const int BLACK = 1;         // player number for Black

// ---------------------------------------------------------------------
// UI COLORS (ANSI escape codes)
// These special text codes tell the console to change text/background
// color. They are plain strings, so no extra library is needed.
// ---------------------------------------------------------------------
const string RESET      = "\033[0m";
const string BOLD       = "\033[1m";
const string RED        = "\033[1;91m";
const string GREEN      = "\033[1;92m";
const string YELLOW     = "\033[1;93m";
const string CYAN       = "\033[1;96m";
const string GRAY       = "\033[90m";
const string LIGHT_BG   = "\033[48;5;180m";   // light square (tan)
const string DARK_BG    = "\033[48;5;137m";   // dark square (brown)
const string LAST_BG    = "\033[48;5;107m";   // squares of the last move (green)
const string CHECK_BG   = "\033[48;5;160m";   // king in check (red)
const string WHITE_FG   = "\033[1;97m";       // White pieces (bright white)
const string BLACK_FG   = "\033[1;30m";       // Black pieces (black)
const string EMPTY_FG   = "\033[38;5;95m";    // the '.' on empty squares

// ---------------------------------------------------------------------
// DATA STRUCTURE 1: 2D ARRAY
// board[row][col] holds one character per square.
// Row 0 is rank 8 (Black's side) and row 7 is rank 1 (White's side).
// Uppercase = White piece, lowercase = Black piece, '.' = empty square.
// A 2D array is used because a chess board is naturally a grid.
// ---------------------------------------------------------------------
char board[SIZE][SIZE];

// ---------------------------------------------------------------------
// Move: stores everything needed to describe (and undo) one move.
// piece    = the piece that moved (saved BEFORE promotion, so undo
//            turns a promoted queen back into a pawn)
// captured = the piece that was on the target square ('.' if none)
// ---------------------------------------------------------------------
struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    char piece;
    char captured;
};

// ---------------------------------------------------------------------
// DATA STRUCTURE 2: STACK (array based)
// Holds the move history. The last move made is always on top,
// so "undo" simply pops the top move (Last In, First Out).
// ---------------------------------------------------------------------
struct MoveStack {
    Move items[MAX_MOVES];
    int top;               // index of the top element, -1 means empty
};

// ---------------------------------------------------------------------
// DATA STRUCTURE 3: QUEUE (circular array of size 2)
// Holds the two players. The player at the front has the turn.
// After a move, the front player is dequeued and enqueued again at
// the back, so turns rotate White -> Black -> White ... (round robin).
// ---------------------------------------------------------------------
struct TurnQueue {
    int items[2];
    int front;
    int rear;
    int count;
};

// ---------------------------------------------------------------------
// DATA STRUCTURE 4: LINKED LIST NODE
// Each node stores one captured piece and a pointer to the next node.
// A linked list is used because we do not know in advance how many
// pieces will be captured, so it grows one node at a time.
// ---------------------------------------------------------------------
struct Node {
    char piece;
    Node* next;
};

// Global game objects
MoveStack history;                     // stack of all moves made
TurnQueue turns;                       // queue of players (turn order)
Node* capturedBy[2] = { NULL, NULL };  // capturedBy[WHITE] = pieces White has taken


// =====================================================================
//                       STACK FUNCTIONS
// =====================================================================

// Makes the stack empty.
void initStack(MoveStack &s) {
    s.top = -1;
}

// Returns true if there are no moves in the stack.
bool isStackEmpty(MoveStack &s) {
    return s.top == -1;
}

// Returns true if the stack cannot hold more moves.
bool isStackFull(MoveStack &s) {
    return s.top == MAX_MOVES - 1;
}

// Puts a move on top of the stack.
void push(MoveStack &s, Move m) {
    if (!isStackFull(s)) {
        s.top++;
        s.items[s.top] = m;
    }
}

// Removes and returns the move on top of the stack.
Move pop(MoveStack &s) {
    Move m = s.items[s.top];
    s.top--;
    return m;
}


// =====================================================================
//                       QUEUE FUNCTIONS
// =====================================================================

// Makes the queue empty.
void initQueue(TurnQueue &q) {
    q.front = 0;
    q.rear = -1;
    q.count = 0;
}

// Adds a player at the back of the queue (circular index).
void enqueue(TurnQueue &q, int player) {
    if (q.count == 2) return;          // queue is full
    q.rear = (q.rear + 1) % 2;
    q.items[q.rear] = player;
    q.count++;
}

// Removes and returns the player at the front of the queue.
int dequeue(TurnQueue &q) {
    int player = q.items[q.front];
    q.front = (q.front + 1) % 2;
    q.count--;
    return player;
}

// Returns the player at the front (the player whose turn it is).
int currentPlayer(TurnQueue &q) {
    return q.items[q.front];
}

// Ends the current turn: the front player goes to the back of the queue.
void nextTurn(TurnQueue &q) {
    int player = dequeue(q);
    enqueue(q, player);
}


// =====================================================================
//                     LINKED LIST FUNCTIONS
// =====================================================================

// Inserts a captured piece at the head of the list.
void addCaptured(Node* &head, char piece) {
    Node* newNode = new Node;
    newNode->piece = piece;
    newNode->next = head;
    head = newNode;
}

// Deletes the first node holding the given piece (used by undo).
void removeCaptured(Node* &head, char piece) {
    Node* current = head;
    Node* previous = NULL;

    while (current != NULL && current->piece != piece) {
        previous = current;
        current = current->next;
    }
    if (current == NULL) return;        // piece not found

    if (previous == NULL) head = current->next;   // removing the head
    else previous->next = current->next;          // removing in the middle/end
    delete current;
}

// Deletes every node in the list to free memory.
void clearList(Node* &head) {
    while (head != NULL) {
        Node* temp = head;
        head = head->next;
        delete temp;
    }
}


// =====================================================================
//                       HELPER FUNCTIONS
// =====================================================================

// Returns true if the piece is White (uppercase letter).
bool isWhitePiece(char p) {
    return p >= 'A' && p <= 'Z';
}

// Returns true if the piece is Black (lowercase letter).
bool isBlackPiece(char p) {
    return p >= 'a' && p <= 'z';
}

// Returns true if the piece belongs to the given player.
bool belongsTo(char p, int player) {
    if (player == WHITE) return isWhitePiece(p);
    return isBlackPiece(p);
}

// Converts an uppercase letter to lowercase (so 'P' and 'p' are both "pawn").
char toLowerCase(char c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

// Returns the player's name as text.
string playerName(int player) {
    if (player == WHITE) return "White";
    return "Black";
}

// Returns the full name of a piece, e.g. 'n' -> "Knight".
string pieceName(char p) {
    char t = toLowerCase(p);
    if (t == 'p') return "Pawn";
    if (t == 'n') return "Knight";
    if (t == 'b') return "Bishop";
    if (t == 'r') return "Rook";
    if (t == 'q') return "Queen";
    return "King";
}

// Returns the value of a piece: pawn 1, knight 3, bishop 3, rook 5, queen 9.
int pieceValue(char p) {
    char t = toLowerCase(p);
    if (t == 'p') return 1;
    if (t == 'n') return 3;
    if (t == 'b') return 3;
    if (t == 'r') return 5;
    if (t == 'q') return 9;
    return 0;   // king (the game ends when it is captured)
}

// Turns board coordinates into chess notation, e.g. (6,4) -> "e2".
string squareName(int row, int col) {
    string s = "";
    s += char('a' + col);
    s += char('0' + (SIZE - row));
    return s;
}

// Turns chess notation into board coordinates, e.g. "e2" -> (6,4).
// Returns false if the text is not a real square.
bool parseSquare(string s, int &row, int &col) {
    if (s.length() != 2) return false;
    char letter = toLowerCase(s[0]);
    char digit = s[1];
    if (letter < 'a' || letter > 'h') return false;
    if (digit < '1' || digit > '8') return false;
    col = letter - 'a';
    row = SIZE - (digit - '0');
    return true;
}

// Removes spaces from the start/end of the input and makes it lowercase.
string cleanInput(string s) {
    int start = 0;
    int end = s.length() - 1;
    while (start <= end && (s[start] == ' ' || s[start] == '\t')) start++;
    while (end >= start && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r')) end--;

    string result = "";
    for (int i = start; i <= end; i++) result += toLowerCase(s[i]);
    return result;
}

// Reads a move like "e2 e4" and fills the four coordinates.
// Returns false if the format or the squares are wrong.
bool readMove(string line, int &fr, int &fc, int &tr, int &tc) {
    int space = line.find(' ');
    if (space == (int)string::npos) return false;

    string from = line.substr(0, space);
    string to = cleanInput(line.substr(space + 1));
    return parseSquare(from, fr, fc) && parseSquare(to, tr, tc);
}


// =====================================================================
//                     SCREEN (UI) FUNCTIONS
// =====================================================================

// Clears the console and moves the cursor to the top-left corner.
void clearScreen() {
    cout << "\033[2J\033[H";
}

// Waits until the user presses Enter (so they can read the screen).
void waitForEnter() {
    cout << GRAY << "\n  Press Enter to continue..." << RESET;
    string dummy;
    getline(cin, dummy);
}

// Prints the game title banner at the top of the screen.
void printTitle() {
    cout << YELLOW;
    cout << "  +==========================================+\n";
    cout << "  |        C O N S O L E    C H E S S        |\n";
    cout << "  +==========================================+\n";
    cout << RESET;
}


// =====================================================================
//                        BOARD FUNCTIONS
// =====================================================================

// Puts all 32 pieces on their standard starting squares.
void setupBoard() {
    string backRow = "rnbqkbnr";
    for (int c = 0; c < SIZE; c++) {
        board[0][c] = backRow[c];          // Black back row
        board[1][c] = 'p';                 // Black pawns
        for (int r = 2; r < 6; r++) {
            board[r][c] = '.';             // empty middle
        }
        board[6][c] = 'P';                 // White pawns
        board[7][c] = backRow[c] - 32;     // White back row (uppercase)
    }
}


// =====================================================================
//                      MOVE RULE FUNCTIONS
// =====================================================================

// Checks that every square BETWEEN start and end is empty.
// Used by rook, bishop and queen, which cannot jump over pieces.
bool isPathClear(int fr, int fc, int tr, int tc) {
    int stepRow = 0, stepCol = 0;
    if (tr > fr) stepRow = 1;
    if (tr < fr) stepRow = -1;
    if (tc > fc) stepCol = 1;
    if (tc < fc) stepCol = -1;

    int r = fr + stepRow;
    int c = fc + stepCol;
    while (r != tr || c != tc) {
        if (board[r][c] != '.') return false;
        r += stepRow;
        c += stepCol;
    }
    return true;
}

// Pawn rules: 1 step forward, 2 steps from the start row,
// or 1 step diagonally forward only when capturing.
bool isValidPawnMove(int fr, int fc, int tr, int tc) {
    char piece = board[fr][fc];
    char target = board[tr][tc];
    int direction = isWhitePiece(piece) ? -1 : 1;   // White goes up, Black goes down
    int startRow = isWhitePiece(piece) ? 6 : 1;
    int dr = tr - fr;
    int dc = tc - fc;

    if (dc == 0 && dr == direction && target == '.')
        return true;                                            // one step
    if (dc == 0 && dr == 2 * direction && fr == startRow &&
        target == '.' && board[fr + direction][fc] == '.')
        return true;                                            // two steps
    if (abs(dc) == 1 && dr == direction && target != '.')
        return true;                                            // diagonal capture
    return false;
}

// Checks if the piece on (fr,fc) is allowed to move to (tr,tc).
// It does NOT check whose turn it is; that is done in playGame().
bool isValidMove(int fr, int fc, int tr, int tc) {
    char piece = board[fr][fc];
    char target = board[tr][tc];

    if (piece == '.') return false;                    // nothing to move
    if (fr == tr && fc == tc) return false;            // must actually move
    if (target != '.' && isWhitePiece(piece) == isWhitePiece(target))
        return false;                                  // cannot capture own piece

    int dr = abs(tr - fr);
    int dc = abs(tc - fc);

    switch (toLowerCase(piece)) {
        case 'p': return isValidPawnMove(fr, fc, tr, tc);
        case 'r': return (dr == 0 || dc == 0) && isPathClear(fr, fc, tr, tc);
        case 'b': return (dr == dc) && isPathClear(fr, fc, tr, tc);
        case 'q': return (dr == 0 || dc == 0 || dr == dc) && isPathClear(fr, fc, tr, tc);
        case 'n': return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
        case 'k': return dr <= 1 && dc <= 1;
    }
    return false;
}


// =====================================================================
//                SEARCHING: find king and detect check
// =====================================================================

// LINEAR SEARCH: checks the squares one by one until the king is found.
// Returns false if the king is not on the board.
bool findKing(int player, int &kingRow, int &kingCol) {
    char king = (player == WHITE) ? 'K' : 'k';
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (board[r][c] == king) {
                kingRow = r;
                kingCol = c;
                return true;
            }
        }
    }
    return false;
}

// Returns true if any enemy piece can move onto this player's king.
bool isKingInCheck(int player) {
    int kr, kc;
    if (!findKing(player, kr, kc)) return false;

    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            char p = board[r][c];
            if (p != '.' && !belongsTo(p, player) && isValidMove(r, c, kr, kc))
                return true;
        }
    }
    return false;
}


// =====================================================================
//                SORTING: captured pieces by value
// =====================================================================

// BUBBLE SORT on the linked list: compares neighbour nodes and swaps
// their pieces until the list is in order (highest value first).
void sortCaptured(Node* head) {
    bool swapped = true;
    while (swapped) {
        swapped = false;
        Node* current = head;
        while (current != NULL && current->next != NULL) {
            if (pieceValue(current->piece) < pieceValue(current->next->piece)) {
                char temp = current->piece;
                current->piece = current->next->piece;
                current->next->piece = temp;
                swapped = true;
            }
            current = current->next;
        }
    }
}

// Adds up the values of all pieces in a captured list.
int totalPoints(Node* head) {
    int total = 0;
    for (Node* current = head; current != NULL; current = current->next)
        total += pieceValue(current->piece);
    return total;
}

// Sorts a captured list and returns it as text, e.g. "q r p p".
string capturedToText(Node* head) {
    sortCaptured(head);
    if (head == NULL) return "-";
    string text = "";
    for (Node* current = head; current != NULL; current = current->next) {
        text += current->piece;
        text += ' ';
    }
    return text;
}

// Shows a full screen with the sorted captured pieces of both players.
void showCaptured() {
    clearScreen();
    printTitle();
    cout << CYAN << "\n  CAPTURED PIECES (sorted by value, bubble sort)\n\n" << RESET;

    for (int player = WHITE; player <= BLACK; player++) {
        sortCaptured(capturedBy[player]);
        cout << BOLD << "  " << playerName(player) << " has captured:\n" << RESET;

        Node* current = capturedBy[player];
        if (current == NULL) cout << GRAY << "     nothing yet\n" << RESET;
        while (current != NULL) {
            cout << "     " << current->piece << "  " << pieceName(current->piece)
                 << "  (" << pieceValue(current->piece) << ")\n";
            current = current->next;
        }
        cout << YELLOW << "     Total points: " << totalPoints(capturedBy[player])
             << RESET << "\n\n";
    }
    waitForEnter();
}


// =====================================================================
//                    MAKING AND UNDOING MOVES
// =====================================================================

// Turns a pawn into a queen when it reaches the last row.
// Returns true if a promotion happened.
bool promotePawn(int row, int col) {
    if (board[row][col] == 'P' && row == 0) {
        board[row][col] = 'Q';
        return true;
    }
    if (board[row][col] == 'p' && row == SIZE - 1) {
        board[row][col] = 'q';
        return true;
    }
    return false;
}

// Moves a piece, saves the move on the stack, records any capture in
// the linked list and handles promotion. Returns the captured piece.
char movePiece(int fr, int fc, int tr, int tc, int player) {
    Move m;
    m.fromRow = fr;  m.fromCol = fc;
    m.toRow = tr;    m.toCol = tc;
    m.piece = board[fr][fc];
    m.captured = board[tr][tc];

    if (m.captured != '.') addCaptured(capturedBy[player], m.captured);

    board[tr][tc] = m.piece;
    board[fr][fc] = '.';
    promotePawn(tr, tc);

    push(history, m);
    return m.captured;
}

// Pops the last move from the stack and puts the board back the way it
// was, including the captured piece. Also gives the turn back.
bool undoMove() {
    if (isStackEmpty(history)) return false;

    Move m = pop(history);
    board[m.fromRow][m.fromCol] = m.piece;      // original piece (un-promotes too)
    board[m.toRow][m.toCol] = m.captured;       // restore captured piece or '.'

    if (m.captured != '.') {
        int mover = isWhitePiece(m.piece) ? WHITE : BLACK;
        removeCaptured(capturedBy[mover], m.captured);
    }

    nextTurn(turns);   // with 2 players, rotating once gives the turn back
    return true;
}

// Returns one move as text, e.g. "Knight g1 -> f3 (x p)".
string moveToText(Move m) {
    string text = pieceName(m.piece) + " " + squareName(m.fromRow, m.fromCol)
                + " -> " + squareName(m.toRow, m.toCol);
    if (m.captured != '.') text += " (x " + pieceName(m.captured) + ")";
    return text;
}

// Shows a full screen with every move, first to last (bottom to top of stack).
void showHistory() {
    clearScreen();
    printTitle();
    cout << CYAN << "\n  MOVE HISTORY (stored in a stack)\n\n" << RESET;

    if (isStackEmpty(history)) cout << GRAY << "  No moves yet.\n" << RESET;
    for (int i = 0; i <= history.top; i++) {
        Move m = history.items[i];
        if (isWhitePiece(m.piece)) cout << WHITE_FG << "  " << (i + 1) << ". White  ";
        else                       cout << GRAY     << "  " << (i + 1) << ". Black  ";
        cout << RESET << moveToText(m) << "\n";
    }
    waitForEnter();
}


// =====================================================================
//                     DRAWING THE GAME SCREEN
// =====================================================================

// Chooses the background color of one square:
// red = king in check, green = last move, otherwise light/dark.
string squareColor(int r, int c, int checkRow, int checkCol) {
    if (r == checkRow && c == checkCol) return CHECK_BG;

    if (!isStackEmpty(history)) {
        Move last = history.items[history.top];
        if ((r == last.fromRow && c == last.fromCol) ||
            (r == last.toRow && c == last.toCol))
            return LAST_BG;
    }
    if ((r + c) % 2 == 0) return LIGHT_BG;
    return DARK_BG;
}

// Prints one square: colored background with the piece letter in the middle.
void printSquare(int r, int c, int checkRow, int checkCol) {
    char p = board[r][c];
    cout << squareColor(r, c, checkRow, checkCol);
    if (p == '.')           cout << EMPTY_FG << " . ";
    else if (isWhitePiece(p)) cout << WHITE_FG << " " << p << " ";
    else                    cout << BLACK_FG << " " << p << " ";
    cout << RESET;
}

// Prints the colored board with row numbers (8 to 1), column letters
// (a to h) and an information panel on the right side.
void printBoard(int player) {
    // find the king of the player to move, if he is in check
    int checkRow = -1, checkCol = -1;
    if (isKingInCheck(player)) findKing(player, checkRow, checkCol);

    // the 8 lines of the side panel (one next to each board row)
    string panel[SIZE];
    panel[0] = BOLD + "Turn:  " + RESET + (player == WHITE ? WHITE_FG : GRAY)
             + playerName(player) + (player == WHITE ? " (UPPERCASE)" : " (lowercase)") + RESET;
    panel[1] = BOLD + "Move:  " + RESET + to_string(history.top + 2);
    panel[2] = BOLD + "Last:  " + RESET
             + (isStackEmpty(history) ? "-" : moveToText(history.items[history.top]));
    panel[3] = "";
    panel[4] = BOLD + "White took: " + RESET + capturedToText(capturedBy[WHITE]);
    panel[5] = BOLD + "Black took: " + RESET + capturedToText(capturedBy[BLACK]);
    panel[6] = BOLD + "Points:  " + RESET + "White " + to_string(totalPoints(capturedBy[WHITE]))
             + "  -  Black " + to_string(totalPoints(capturedBy[BLACK]));
    panel[7] = (checkRow != -1) ? RED + "CHECK! " + playerName(player) + " king is attacked!" + RESET : "";

    cout << "\n      a  b  c  d  e  f  g  h\n";
    for (int r = 0; r < SIZE; r++) {
        cout << "   " << (SIZE - r) << " ";
        for (int c = 0; c < SIZE; c++) printSquare(r, c, checkRow, checkCol);
        cout << " " << (SIZE - r) << "     " << panel[r] << "\n";
    }
    cout << "      a  b  c  d  e  f  g  h\n\n";
}

// Redraws the whole game screen: title, board, panel and a message line.
// isError = true shows the message in red, otherwise in green.
void drawScreen(int player, string message, bool isError) {
    clearScreen();
    printTitle();
    printBoard(player);

    if (message != "") {
        if (isError) cout << RED << "  [!] " << message << RESET << "\n";
        else         cout << GREEN << "  >> " << message << RESET << "\n";
    }
    cout << GRAY << "  Commands: undo | history | captured | quit\n" << RESET;
}


// =====================================================================
//                         GAME AND MENU
// =====================================================================

// Puts everything back to the start for a new game.
void resetGame() {
    setupBoard();
    initStack(history);
    initQueue(turns);
    enqueue(turns, WHITE);    // White moves first
    enqueue(turns, BLACK);
    clearList(capturedBy[WHITE]);
    clearList(capturedBy[BLACK]);
}

// Main game loop: reads commands/moves, checks them and plays them.
void playGame() {
    resetGame();
    string line;
    string message = "White moves first. Type a move like: e2 e4";
    bool isError = false;

    while (true) {
        int player = currentPlayer(turns);
        drawScreen(player, message, isError);
        message = "";
        isError = false;

        cout << BOLD << "  " << playerName(player) << " > " << RESET;
        if (!getline(cin, line)) return;     // input ended
        line = cleanInput(line);

        // ----- special commands -----
        if (line == "quit") return;
        if (line == "undo") {
            if (undoMove()) message = "Last move undone.";
            else { message = "There is no move to undo."; isError = true; }
            continue;
        }
        if (line == "history")  { showHistory();  continue; }
        if (line == "captured") { showCaptured(); continue; }

        // ----- normal move -----
        int fr, fc, tr, tc;
        isError = true;                      // assume an error until the move is played
        if (!readMove(line, fr, fc, tr, tc)) {
            message = "Invalid input! Type two squares like: e2 e4";
            continue;
        }
        char piece = board[fr][fc];
        if (piece == '.') {
            message = "There is no piece on " + squareName(fr, fc) + ".";
            continue;
        }
        if (!belongsTo(piece, player)) {
            message = "That is your opponent's piece. Move your own piece.";
            continue;
        }
        if (!isValidMove(fr, fc, tr, tc)) {
            message = "Illegal move: a " + pieceName(piece) + " cannot go from "
                    + squareName(fr, fc) + " to " + squareName(tr, tc) + ".";
            continue;
        }

        char captured = movePiece(fr, fc, tr, tc, player);
        isError = false;
        message = playerName(player) + " played " + moveToText(history.items[history.top]);
        if (board[tr][tc] != piece) message += "  - Pawn promoted to Queen!";

        if (toLowerCase(captured) == 'k') {   // king captured -> game over
            drawScreen(player, "The king is captured! " + playerName(player) + " WINS!", false);
            cout << YELLOW << "\n  *****  " << playerName(player) << " is the winner!  *****\n" << RESET;
            waitForEnter();
            return;
        }

        nextTurn(turns);   // round robin: other player's turn
    }
}

// Shows the rules and the commands.
void showHowToPlay() {
    clearScreen();
    printTitle();
    cout << CYAN << "\n  HOW TO PLAY\n\n" << RESET;
    cout << "  " << WHITE_FG << "White" << RESET << " = UPPERCASE   P R N B Q K\n";
    cout << "  " << GRAY << "Black" << RESET << " = lowercase   p r n b q k\n";
    cout << "  P Pawn   R Rook   N Knight   B Bishop   Q Queen   K King\n";
    cout << "  '.' is an empty square. White moves first.\n\n";
    cout << BOLD << "  Moving:" << RESET << " type the start and end square, e.g.  e2 e4\n\n";
    cout << BOLD << "  Commands during the game:\n" << RESET;
    cout << "    undo      take back the last move\n";
    cout << "    history   show all moves\n";
    cout << "    captured  show captured pieces sorted by value\n";
    cout << "    quit      go back to the menu\n\n";
    cout << BOLD << "  Rules:\n" << RESET;
    cout << "    - A pawn reaching the last row becomes a Queen.\n";
    cout << "    - No castling and no en passant.\n";
    cout << "    - " << RED << "Red square" << RESET << " = king in check, "
         << GREEN << "green squares" << RESET << " = last move.\n";
    cout << "    - Capture the enemy king to win!\n";
    waitForEnter();
}

// Prints the main menu box.
void printMenu() {
    clearScreen();
    printTitle();
    cout << "  |                                          |\n";
    cout << "  |      " << GREEN << "1." << RESET << "  Play                            |\n";
    cout << "  |      " << GREEN << "2." << RESET << "  How to Play                     |\n";
    cout << "  |      " << GREEN << "3." << RESET << "  Exit                            |\n";
    cout << "  |                                          |\n";
    cout << YELLOW << "  +==========================================+\n" << RESET;
    cout << BOLD << "\n  Choose (1-3): " << RESET;
}

// Shows the main menu until the user chooses Exit.
int main() {
    system("");   // turns on color support in the Windows console

    string choice;
    while (true) {
        printMenu();
        if (!getline(cin, choice)) break;
        choice = cleanInput(choice);

        if (choice == "1") playGame();
        else if (choice == "2") showHowToPlay();
        else if (choice == "3") break;
    }

    clearList(capturedBy[WHITE]);   // free linked list memory
    clearList(capturedBy[BLACK]);
    clearScreen();
    cout << "  Thanks for playing. Goodbye!\n";
    return 0;
}
