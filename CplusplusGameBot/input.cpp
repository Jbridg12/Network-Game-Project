struct Button_State {
	bool is_down;
	bool changed;
};


enum {
	BUTTON_W,
	BUTTON_A,
	BUTTON_S,
	BUTTON_D,
	BUTTON_Z,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_SPACE,
	BUTTON_COUNT
};


struct Input_State {
	Button_State buttons[BUTTON_COUNT];
};