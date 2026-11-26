#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;

//Singleton: GameManager
class GameManager {
private:
    int wave;
    int asteroidsDestroyed;
    string gameState;
    int reserveAmmo;
    int medkit;

    GameManager() {
        wave = 1;
        asteroidsDestroyed = 0;
        gameState = "Start";
        reserveAmmo = 10;
        medkit = 3;
    }

public:
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    static GameManager& getInstance() {
        static GameManager instance;
        return instance;
    }

    int getWave() { return wave; }
    int getAsteroidsDestroyed() { return asteroidsDestroyed; }
    string getGameState() { return gameState; }
    int getReserveAmmo() { return reserveAmmo; }
    int getMedkit() { return medkit; }

    void setGameState(string state) {
        gameState = state;
    }

    int takeReserveAmmo(int need) {
        int taken = need;
        if (taken > reserveAmmo) {
            taken = reserveAmmo;
        }
        reserveAmmo -= taken;
        return taken;
    }

    bool useMedkit() {
        if (medkit <= 0) {
            return false;
        }
        medkit--;
        return true;
    }

    void addAsteroidDestroyed() {
        asteroidsDestroyed++;
        if (asteroidsDestroyed % 3 == 0) {
            medkit++;
            cout << "[GameManager] Bonus! Medkit +1" << endl;
        }
        if (asteroidsDestroyed % 4 == 0) {
            wave++;
            reserveAmmo += 5;
            cout << "[GameManager] Wave " << wave
                 << " dimulai! Ammo cadangan +5" << endl;
        }
    }
};

//Asteroid
class Asteroid {
private:
    int id;
    int damage;
    int distance;

public:
    Asteroid(int id, int distance, int damage) {
        this->id = id;
        this->damage = damage;
        this->distance = distance;
    }

    void moveCloser() { distance--; }
    bool hasArrived() { return distance <= 0; }

    int getId() { return id; }
    int getDamage() { return damage; }
    int getDistance() { return distance; }
};

enum class ProjectileState { Ready, Fired, Empty };

class Projectile {
private:
    int id;
    ProjectileState state;

public:
    Projectile(int id) {
        this->id = id;
        this->state = ProjectileState::Ready;
    }

    void fire() {
        state = ProjectileState::Fired;
        cout << "[Projectile] Laser " << id << " ditembakkan." << endl;
    }

    // Objek tidak dihapus, hanya ditandai kosong
    void reset() {
        state = ProjectileState::Empty;
    }

    // Diisi ulang agar bisa dipakai lagi
    void recharge() {
        state = ProjectileState::Ready;
    }

    ProjectileState getState() { return state; }
    int getId() { return id; }
};

//Object Pool : for laser
class ProjectilePool {
private:
    vector<Projectile> projectiles;

public:
    ProjectilePool(int size) {
        for (int i = 0; i < size; i++) {
            projectiles.push_back(Projectile(i + 1));
        }
    }

    Projectile* getProjectile(ProjectileState state) {
        for (int i = 0; i < (int)projectiles.size(); i++) {
            if (projectiles[i].getState() == state) {
                return &projectiles[i];
            }
        }
        return nullptr;
    }

    int count(ProjectileState state) {
        int total = 0;
        for (int i = 0; i < (int)projectiles.size(); i++) {
            if (projectiles[i].getState() == state) {
                total++;
            }
        }
        return total;
    }

    int getSize() {
        return (int)projectiles.size();
    }
};

//Receiver: Player
class Player {
private:
    int hp;
    int maxHp;
    ProjectilePool* pool;

public:
    Player(ProjectilePool* pool) {
        this->pool = pool;
        maxHp = 80;
        hp = maxHp;
    }

    bool attack() {
        Projectile* laser = pool->getProjectile(ProjectileState::Ready);
        if (laser == nullptr) {
            cout << "[Player] Ammo laser kosong! Lakukan reload." << endl;
            return false;
        }
        laser->fire();
        return true;
    }

    bool reload() {
        int need = pool->count(ProjectileState::Empty);
        if (need == 0) {
            cout << "[Player] Ammo laser sudah penuh." << endl;
            return false;
        }
        int taken = GameManager::getInstance().takeReserveAmmo(need);
        if (taken == 0) {
            cout << "[Player] Ammo cadangan habis!" << endl;
            return false;
        }
        for (int i = 0; i < taken; i++) {
            pool->getProjectile(ProjectileState::Empty)->recharge();
        }
        cout << "[Player] Reload +" << taken << " ammo laser." << endl;
        return true;
    }

    bool heal() {
        if (hp >= maxHp) {
            cout << "[Player] HP sudah penuh." << endl;
            return false;
        }
        if (!GameManager::getInstance().useMedkit()) {
            cout << "[Player] Medkit habis!" << endl;
            return false;
        }
        int before = hp;
        hp += 15;
        if (hp > maxHp) {
            hp = maxHp;
        }
        cout << "[Player] Heal +" << hp - before << " HP." << endl;
        return true;
    }

    void takeDamage(int damage) {
        hp -= damage;
        if (hp < 0) {
            hp = 0;
        }
        cout << "[Player] HP -" << damage << endl;
    }

    bool isAlive() { return hp > 0; }
    int getHp() { return hp; }
    int getMaxHp() { return maxHp; }
    int getAmmo() { return pool->count(ProjectileState::Ready); }
    int getMaxAmmo() { return pool->getSize(); }
};

//Command Interface
class Command {
public:
    virtual ~Command() {}
    virtual bool execute() = 0;
    virtual string getName() = 0;
};

//Concrete Command
class AttackCommand : public Command {
private:
    Player* player;
public:
    AttackCommand(Player* p) : player(p) {}
    bool execute() override {
        return player->attack();
    }
    string getName() override { return "Attack"; }
};

class ReloadCommand : public Command {
private:
    Player* player;
public:
    ReloadCommand(Player* p) : player(p) {}
    bool execute() override {
        return player->reload();
    }
    string getName() override { return "Reload"; }
};

class HealCommand : public Command {
private:
    Player* player;
public:
    HealCommand(Player* p) : player(p) {}
    bool execute() override {
        return player->heal();
    }
    string getName() override { return "Heal"; }
};

//Invoker
class InputHandler {
private:
    vector<char> keys;
    vector<Command*> commands;

public:
    void bindKey(char key, Command* command) {
        for (int i = 0; i < (int)commands.size(); i++) {
            if (commands[i] == command) {
                keys[i] = key;
                return;
            }
        }
        keys.push_back(key);
        commands.push_back(command);
    }

    bool handleInput(char input) {
        for (int i = 0; i < (int)keys.size(); i++) {
            if (keys[i] == input) {
                return commands[i]->execute();
            }
        }
        cout << "Unknown key" << endl;
        return false;
    }

    void showKeys() {
        cout << "Kontrol:" << endl;
        for (int i = 0; i < (int)keys.size(); i++) {
            cout << keys[i] << " = " << commands[i]->getName() << endl;
        }
        cout << "q = quit" << endl;
    }
};

//game
class Game {
private:
    ProjectilePool* pool;
    Player* player;
    InputHandler* input;
    vector<Asteroid> asteroids; 
    int nextAsteroidId;
    int turn;

    void spawnAsteroid(int distance = 3) {
        int damage = 15;
        int wave = GameManager::getInstance().getWave();
        if (wave > 2) {
            damage += (wave - 6) * 2;
        }
        asteroids.push_back(Asteroid(nextAsteroidId, distance, damage));
        cout << "[Game] Asteroid " << nextAsteroidId
             << " muncul (jarak " << distance << ")." << endl;
        nextAsteroidId++;
    }

    void updateWorld() {
        GameManager& gm = GameManager::getInstance();
        turn++;

        Projectile* laser = pool->getProjectile(ProjectileState::Fired);
        if (laser != nullptr) {
            if (asteroids.empty()) {
                cout << "[Game] Laser " << laser->getId()
                     << " meleset, tidak ada asteroid." << endl;
            } else if(rand() % 100 < 55) { 
                cout << "[Game] Laser " << laser->getId()
                     << " menghancurkan Asteroid "
                     << asteroids[0].getId() << "!" << endl;
                asteroids.erase(asteroids.begin());
                gm.addAsteroidDestroyed();
            } else {
                cout << "[Game] Laser " << laser->getId()
                     << " meleset, asteroid " << asteroids[0].getId()
                     << " lolos." << endl;
            }
            laser->reset();   // kembali ke pool
        }

        for (int i = 0; i < (int)asteroids.size(); i++) {
            asteroids[i].moveCloser();
        }

        while (!asteroids.empty() && asteroids[0].hasArrived()) {
            cout << "[Game] Asteroid " << asteroids[0].getId()
                 << " menabrak pesawat!" << endl;
            player->takeDamage(asteroids[0].getDamage());
            asteroids.erase(asteroids.begin());
        }

        // 4. Asteroid baru: wave 1-3 tiap 2 giliran,
        //    wave 4-6 dua kali tiap 3 giliran,
        //    wave 7 ke atas tiap giliran.
        //    Jika layar kosong, langsung muncul 1
        bool timeToSpawn;
        if (gm.getWave() < 4) {
            timeToSpawn = (turn % 2 == 0);
        } else if (gm.getWave() < 7) {
            timeToSpawn = (turn % 3 != 0);
        } else {
            timeToSpawn = true;
        }
        if (timeToSpawn || asteroids.empty()) {
            spawnAsteroid();
        }
    }

    void showStatus() {
        GameManager& gm = GameManager::getInstance();
        cout << "\n========== STATUS ==========" << endl;
        cout << "Wave           : " << gm.getWave() << endl;
        cout << "Game State     : " << gm.getGameState() << endl;
        cout << "HP             : " << player->getHp()
             << "/" << player->getMaxHp() << endl;
        cout << "Ammo laser     : " << player->getAmmo()
             << "/" << player->getMaxAmmo()
             << " (cadangan: " << gm.getReserveAmmo() << ")" << endl;
        cout << "Medkit         : " << gm.getMedkit() << endl;
        cout << "Asteroid hancur: " << gm.getAsteroidsDestroyed() << endl;
        cout << "Asteroid aktif : " << asteroids.size() << endl;
        for (int i = 0; i < (int)asteroids.size(); i++) {
            cout << "  - Asteroid " << asteroids[i].getId()
                 << " (jarak " << asteroids[i].getDistance() << ")" << endl;
        }
        cout << "============================" << endl;
    }

public:
    Game(ProjectilePool* pool, Player* player, InputHandler* input) {
        this->pool = pool;
        this->player = player;
        this->input = input;
        nextAsteroidId = 1;
        turn = 0;
    }

    void run() {
        GameManager& gm = GameManager::getInstance();
        gm.setGameState("Playing");

        cout << "=== SPACE SURVIVAL MISSION ===" << endl;
        input->showKeys();

        // Awal game langsung ada 2 asteroid
        spawnAsteroid(3);
        spawnAsteroid(4);
        showStatus();

        char key;
        while (gm.getGameState() == "Playing") {
            cout << "\nInput: ";
            if (!(cin >> key)) {
                break;
            }

            if (key == 'q') {
                gm.setGameState("Quit");
                cout << "Game exit." << endl;
                break;
            }

            cout << endl;
            if (!input->handleInput(key)) {
                continue;   
            }

            updateWorld();

            if (!player->isAlive()) {
                gm.setGameState("Game Over");
            } else if (gm.getWave() > 5) {
                gm.setGameState("Victory");
            }
            showStatus();
        }

        if (gm.getGameState() == "Game Over") {
            cout << "\n*** GAME OVER ***" << endl;
            cout << "Wave tercapai   : " << gm.getWave() << endl;
            cout << "Asteroid hancur : " << gm.getAsteroidsDestroyed()
                 << endl;
        }

        if (gm.getGameState() == "Victory") {
            cout << "\n*** MISSION COMPLETE ***" << endl;
            cout << "5 wave berhasil diselesaikan!" << endl;
            cout << "Asteroid hancur : " << gm.getAsteroidsDestroyed()
                 << endl;
        }
    }
};


int main() {
    srand(time(0));
    ProjectilePool pool(6);
    Player player(&pool);

    // Concrete Command
    Command* attackCmd = new AttackCommand(&player);
    Command* reloadCmd = new ReloadCommand(&player);
    Command* healCmd = new HealCommand(&player);

    // Invoker: remap here
    InputHandler inputHandler;
    inputHandler.bindKey('j', attackCmd);
    inputHandler.bindKey('r', reloadCmd);
    inputHandler.bindKey('h', healCmd);

    // Game
    Game game(&pool, &player, &inputHandler);
    game.run();

    delete attackCmd;
    delete reloadCmd;
    delete healCmd;

    return 0;
}