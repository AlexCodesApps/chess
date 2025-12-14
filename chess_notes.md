Bishop Movement pattern
1,2 					7,2
	2,3 			6,3
		3,4 	5,4
			4,5
		3,6 	5,6
	2,7 			6,7

In general where (x, y) is origin, (x2, y2) is second position
if abs(x - x2) == abs(y - y2), then its in the bishops line of sight,
provided there is no obstruction.

ChessBoard index map
 0  1  2  3  4  5  6  7
 8  9 10 11 12 13 14 15
16 17 18 19 20 21 22 23
24 25 26 27 28 29 30 31
32 33 34 35 36 37 38 39
40 41 42 43 44 45 46 47
48 49 50 51 52 53 54 55
56 57 58 59 60 61 62 63
Therefore, when a pawn advances two spaces, abs(idx1 - idx2) == 16
Likewise, when a pawn advances one space, abs(idx1 - idx2) == 8.
This is irrespective of direction.
We can use this to detect if a pawn is eligible for getting en-passant treatment.
