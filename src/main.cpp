#include "game.hpp"

int main() {
	spellbook::game.startup();
	spellbook::game.run();
	spellbook::game.shutdown();

	return 0;
}
