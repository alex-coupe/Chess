#pragma once
#include "Piece.h"

class Pawn : public Piece {
public:
	Pawn(int x, int y, const std::string spritename);
private:
	//Pawns can go two spaces in their first move so this will be used to populate move list each selection
	bool firstMove = true;
};