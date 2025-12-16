try to parse unknown commands by skipping ahead to a known command.
a unknown command argument cannot be handled this way.

FEN encoding
start from rows a-h, col 8-1
rows are seperated by /
white is uppercase, black is lowercase

letters [r, n, b, q, k, p] used for pieces.
empty squares are represented by their adjacent count.

next, [w or b] for white or blacks turn

next up castling rights
1. white queen side K
2. white king side Q
3. black queen side k
4. black king side q
if none then just '-'.

next is the location [a-h][1-8] of the last pawn that moved
two spaces (can get enpassanted), or '-' if nothing.

second last is the 50move draw counter, starts at 0.

last is the turn count, starts at 1 and increments when black's turn finishes.
