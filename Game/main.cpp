#include <ql.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <Windows.h>
#include <iostream>
#include <string>
using namespace std;
namespace info { // пространство имен для счетчика и цвета фона/экрана
	ql::rgb entity_color = ql::rgb(220, 220, 220);
	ql::rgb background_color = ql::rgb(40, 40, 40);
	int player_score = 0;
	int opponent_score = 0;
};
enum class direction {  // скопированные перечисления ( в данном случае направление игровых плиток) К ним обращае
	up,
	down
};
enum class bounce_direction { // перечислители координат, направление отскока шара
	x,
	y,
	none 
};


struct hud { //текстовая структура счетчика игры 
	sf::Text player_score; 
	sf::Text opponent_score;
	sf::Font font;

	void init_text(sf::Font& font, int character_size, ql::rgb color, ql::vector2f screen_dimension) { // u32 - unsigned int 
		player_score.setFont(font);
		player_score.setCharacterSize(character_size);
		player_score.setFillColor(color);
		player_score.setPosition(400, 30);
		player_score.setString("0");
		opponent_score.setFont(font);
		opponent_score.setCharacterSize(character_size);
		opponent_score.setFillColor(color);
		opponent_score.setPosition(screen_dimension.x - 400, 30);
		opponent_score.setString("0");
	}
	void draw(ql::draw_object& draw_object) const { // прорисовка счетчика на экране
		draw_object.draw(player_score); 
		draw_object.draw(opponent_score);
	}
};

struct rectangle { // структура игровых "плиток"
	ql::vector2f position;
	ql::vector2f dimension; 
	bounce_direction last_direction; // отвечает за направление плиток
	ql::f64 speed = 600; // . Дефолт -  300 скорость
	void make_shape() { //   set переписывает пред позицию, размер и цвет плиток
		shape.setPosition(position); 
		shape.setSize(dimension);
		shape.setFillColor(info::entity_color);
	}
	void move(ql::f64 frame_time, direction direction, ql::vector2f scdimension) { //в зависимости от направления, меняется положение плиток
		if (direction == direction::up) { // обращаемся к перечислителю за значением направления
			this->position.y -= frame_time * speed; // если плитка движется вверх, то мы вычитаем произведение ускорения на фр.тайм (поскольку декартова система координат)
			if (position.y < 0) position.y = 0; // верхняя стенка для плиток 
			shape.setPosition(position); // обновляем прорисовку плиток 
		}
		else if (direction == direction::down) { 
			position.y += frame_time * speed;
			
			if (position.y > scdimension.y -  dimension.y) position.y = scdimension.y - dimension.y; 

			shape.setPosition(position);
		}
	}
	void move_to_ball(ql::f64 frame_time,ql::vector2f ball_position, ql::vector2f scdimension) { //функция для плитки "противника", которая фиксирует направление плитки за шаром
		auto center_y = (position.y + dimension.y / 2); //определяет центр плитки. 
		if (center_y < ball_position.y) {
		move(frame_time, direction::down, scdimension);  
	}
		if (center_y > ball_position.y) { 
			move(frame_time, direction::up,scdimension );
		}
	}
	void draw(ql::draw_object& draw_object) const { 
		draw_object.draw(shape);
	}
	sf::RectangleShape shape;  
};
struct circle { // структура для шара
	ql::f32 ball_speed_over_time = 400; 
	ql::vector2f position; // вектор Х,У 
	ql::vector2f velocity = { ball_speed_over_time, 250 }; // Скорость в рамках декартовой системы - передвижение тела по х и у. В нашем случае, тело двигается по горизонтали быстрее, чем по вертикали 400 200 дефолт скорость
	ql::rgb color = info::entity_color; 
	ql::f64 cooldown = 0.07;  
	ql::f32 radius; 
	ql::clock respawn_cooldown; 

	void make_shape() { //Формирование  радиуса, позиции, а также цвета шарика.
		shape.setRadius(radius);
		shape.setOrigin(radius, radius);
		shape.setPosition(position);
		shape.setFillColor(color);
	}
	void move(ql::f64 frame_time) {
		if (respawn_cooldown.elapsed_f() > 0.7) {
			position -= velocity * frame_time; //обновляем позицию от скорости и картинки. Получаем плавное передвижение 
		}
	}
	bool is_bounce_x_allowed() const { // проверяем на возможность отскока,  возвращаем  кулдаун при  столкновении об стенку
		return bounce_cooldown_x.elapsed_f() > cooldown;
	}
	
	bool is_bounce_y_allowed() const {
		return bounce_cooldown_y.elapsed_f() > cooldown;
	}
	void update_speed(ql::f64 frame_time) { // Для усложнения игры, добавил функцию ускорения шара со временем
		this->ball_speed_over_time += frame_time * 5; 
		auto sign = velocity.x < 0; // проверка на знак. 
		velocity.x =ball_speed_over_time;
		if (sign) velocity.x *= -1;
	}
	void bounce_if_possible(bounce_direction direction, bool randomness) { // добавил еще одну характеристику к изменению скорости.	При ударе о плитку, она приобретает рандомное значение
		if (direction == bounce_direction::x) {
			if (is_bounce_x_allowed()) {
				bounce_cooldown_x.reset(); // если ударяется об плитку, 
				velocity.x *= -1;
				if (randomness) {
					this->velocity.y += ql::random(-1000, 100); // -100 100 default. 
				}
				ql::println(velocity); 
			}

		}
		else {
			if (is_bounce_y_allowed()) {
				bounce_cooldown_y.reset();
				velocity.y *= -1;
			}
		}
	}
	void respawn(ql::vector2f screen_dimension) { // респавн  шарика
		position = screen_dimension / 2;
		respawn_cooldown.reset(); // сбрасываем счетчик респавна после попадания в поле 
		velocity = { ball_speed_over_time, ql::random(-	250,250) }; // 100 100 default.
		if (ql::random_event()) { 
			velocity.x *= -1;
		}
	}
	void check_collision_screen(ql::vector2f screen_dimension, hud& hud) { // проверка на возможность получения очка 

		bool score_happened = false; 
		if (position.x - radius < 0) { 
			bounce_if_possible(bounce_direction::x, true);
			++info::opponent_score;
			score_happened = true;
		}
		if (position.x + radius > screen_dimension.x){ //аналогично, но уже для игрока
			++info::player_score;
			score_happened = true;
		}
		if (score_happened) { // если кто-то попал игровым мячом в край поля противника, то позиция шара возвращается на начальную, а также обновляется счетчик 
			position = screen_dimension / 2; //  ЧИТ 
			respawn_cooldown.reset();
			hud.player_score.setString(ql::to_string(info::player_score));
			hud.opponent_score.setString(ql::to_string(info::opponent_score));
		}
		if (position.y - radius < 0 || position.y + radius > screen_dimension.y) {  // проверка на расположение шара относительно оси у.  при достижении границ происходит отскок
			bounce_if_possible(bounce_direction::y, true);
		}
	}
	void check_collision_rectangle( rectangle& rectangle) { // проверка на вхождение в поле плиток/ в слепую область плиток
		bool collision_y = (position.y > rectangle.position.y && position.y -  radius <(rectangle.position.y + rectangle.dimension.y)); 
		bool collision_x = (position.x > rectangle.position.x && position.x -  radius <(rectangle.position.x + rectangle.dimension.x));
		if (collision_y && collision_x) {
//			this->color = ql::rgb(255, 100, 255); // убрать в случае необходимости
			if (rectangle.last_direction == bounce_direction::y) {
				bounce_if_possible(bounce_direction::x, true);
			}
			else if (rectangle.last_direction == bounce_direction::x) {
				bounce_if_possible(bounce_direction::y, true);
			}
			rectangle.last_direction = bounce_direction::none;
		}
		else if (collision_y) {
//			color = ql::rgb(100, 100, 255); // убрать в случае необходимости
			rectangle.last_direction = bounce_direction::y;
		}
		else if (collision_x) {
//			color = ql::rgb(255, 100, 100); // убрать в случае необходимости
			rectangle.last_direction = bounce_direction::x;
		}
		else {
//			color = info::entity_color; // убрать в случае необходимости
			rectangle.last_direction = bounce_direction:: none;
		}
	}

	void draw(ql::draw_object& draw_object) const { // прорисовка шара 
		draw_object.draw(shape);
	}
	ql::clock bounce_cooldown_x;
	ql::clock bounce_cooldown_y; // кулдаун по координатам х у
	sf::CircleShape shape;
	bounce_direction last_direction;
};


struct game_state : ql::base_state { // скелет состояния игры
	void init() override { // 1 этап, инициализация (обработка данных). На вход получаем счетчик, а также худ 
		info::player_score = 0;
		info::opponent_score = 0; // если мы не будем обнулять счетчик, то он будет со старыми значениями
		get_font("bauhs");
		hud.init_text(get_font("bauhs"), 50, info::entity_color, dimension());
		clear_color = info::background_color;

		player.dimension = { 30,80 }; // параметры и место плиток
		player.position = { 30,30 };
		player.make_shape();

		opponent.dimension = player.dimension;
		opponent.position = { dimension().x - 30 - opponent.dimension.x,30};
		opponent.make_shape();


		ball.radius = 10; //15 - дефолт. Задаем параметры и место шара
		ball.position = dimension() / 2;
		ball.make_shape();
	} 
	void updating() override { // 2 этап. Обновление 
		if (event.key_released(sf::Keyboard::Escape)) { // проверка на сценарии: нажата кнопка esc/w/s
			pop_this_state(); // пропускаем это состояние, если нажата кнопка 
		}
		auto frame_time = this->frame_time().secs_f(); // отвечает за плавность движения (за счёт постоянного контроля состояния клавиатуры в секундах)
		if (event.key_holding(sf::Keyboard::W)) {  // назначаем движение вверх, если нажата w. Аналогично для s
			player.move(frame_time, direction::up, dimension());
		}
		if (event.key_holding(sf::Keyboard::S)) {
			player.move(frame_time, direction::down, dimension());
		}
		opponent.move_to_ball(frame_time, ball.position, dimension());
		opponent.make_shape();
		ball.update_speed(frame_time); // обновляем скорость шара за счет постоянного контроля происходящего на экране, аналогично со счетчиком и коллизицей
		ball.move(frame_time); 
		ball.check_collision_screen(dimension(), hud);
		ball.check_collision_rectangle(player);
		ball.check_collision_rectangle(opponent);
		ball.make_shape();
	}
	void drawing() override { // 3 этап. Прорисовка  (без этого этапа кадры на экране не менялись)
		draw(player); 
		draw(ball);
		draw(hud);
		draw(opponent);
	}

	rectangle player; // назначаем объекты
	rectangle opponent;
	circle ball;
	hud hud;
};

struct button { // кнопки меню.
	sf::RectangleShape background;
	sf::Text text;
	bool hovering = false;
	bool clicked = false;
	void init(sf::Font& font, std::string text, ql::vector2f screen_dimension, ql::f32 y) {
		background.setSize(ql::vector2f(400, 100)); // параметры для кнопок
		background.setPosition((screen_dimension.x - background.getSize().x) / 2, y);
	//	this->background.setFillColor(ql::rgb(50, 50, 50));
		background.setFillColor(info::background_color);
		background.setOutlineThickness(5);
		background.setOutlineColor(info::entity_color);

		auto center = ql::vector2f(background.getPosition()) + ql::vector2f(background.getSize()) / 2;
		this->text.setFont(font);
		this->text.setCharacterSize(42);//28
		this->text.setFillColor(info::entity_color);
		this->text.setString(text);
		auto hitbox = ql::text_hitbox(this->text);
		auto distance = center - (hitbox.position + hitbox.dimension / 2);
		this->text.move(distance);

	};
	void update(ql::event_info& event) { // проверка на взаимодействие с кнопками 
		hovering = false;
		clicked = false;
		auto mouse = event.mouse_position();
		hovering =(mouse.x > background.getPosition().x && mouse.x < background.getPosition().x + background.getSize().x &&
			mouse.y > background.getPosition().y && mouse.y < background.getPosition().y + background.getSize().y);

		if (hovering) {
			text.setFillColor(info::entity_color.inverted());
			background.setFillColor(info::background_color.inverted());
			background.setOutlineColor(info::entity_color.inverted());
		}
		else {
			text.setFillColor(info::entity_color);
			background.setFillColor(info::background_color);
			background.setOutlineColor(info::entity_color);
		}
		
		if (hovering && event.left_mouse_clicked()) {
			clicked = true;

		}
	
	}
	void draw(ql::draw_object & draw_object) const { // прорисовка
		draw_object.draw(this->background);
		draw_object.draw(this->text);
	}
};
struct menu_state : ql::base_state { // скелет меню (которое постоянно ссылается к первому скелету)

	void init() override { // параметры для кнопок, добавление сценариев, если пользователь нажал кнопки
		clear_color = ql::rgb(100, 100, 100);
		info_button.init(this->get_font("bauhs"), "Made by Daniil N",dimension(), 100);
		play_button.init(this->get_font("bauhs"), "PLAY", dimension(), 300);
		exit_button.init(this->get_font("bauhs"), "EXIT", dimension(), 500);
	}
	void updating() override {
		info_button.update(event);
		play_button.update(event);
		exit_button.update(event);
		
		if (info_button.clicked) {

		}
		if (exit_button.clicked) {
			pop_this_state();
		}

		if (play_button.clicked) {
			add_state<game_state>();
		}
	}
	void drawing() override {
		draw(info_button);

		draw(play_button); 

		draw(exit_button);
	}
	button info_button;
	button play_button;
	button exit_button;

};
int main() { 
	ql::framework framework;
	sf::Music music;
	music.openFromFile("audio/gamem.wav");
	music.setVolume(50);
	music.play();
	music.setLoop(true);
	setlocale(LC_ALL, "Russian");
	cout << "Здесь можно наблюдать изменение скорости шара при столкновении с плитками! \n";
	framework.set_dimension({ 1280,720 }); // назначаем разрешение окна игры
	framework.set_title("INSANE PONG"); 
	framework.set_style(sf::Style::Close); // ограничивае возможности
	framework.add_font("bauhs", "resources/bauhs.ttf");
	framework.add_state<menu_state>(); 
	framework.game_loop();
}