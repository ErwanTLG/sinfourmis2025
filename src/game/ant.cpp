#include "game/ant.hpp"

unsigned int Ant::next_id = 0;

Ant::Ant(Queen *queen, fourmi_etat &&etat) : current_Node(queen->get_current_node()), queen(queen) {
    etat.result = -1;
    this->etat = etat;
    max_food = queen->get_stat(Queen::Stat::FOOD);
    max_water = queen->get_stat(Queen::Stat::WATER);
    assert(current_Node != nullptr);
    current_Node->add_ant(this);
    id = next_id++;
}

Ant::Ant(Queen *queen) : current_Node(queen->get_current_node()), queen(queen) {
    etat = queen->default_fourmi_etat();
    max_food = queen->get_stat(Queen::Stat::FOOD);
    max_water = queen->get_stat(Queen::Stat::WATER);
    assert(current_Node != nullptr);
    current_Node->add_ant(this);
    id = next_id++;
}

Ant::Ant(const Ant &&ant) noexcept
    : current_Node(ant.current_Node), queen(ant.queen), etat(ant.etat), max_water(ant.max_water),
      max_food(ant.max_food), action_state(ant.action_state), progress(ant.progress),
      current_edge(ant.current_edge) {}

Ant &Ant::operator=(const Ant &&ant) noexcept {
    queen = ant.queen;
    current_Node = ant.current_Node;
    max_water = ant.max_water;
    max_food = ant.max_food;
    etat = ant.etat;
    action_state = ant.action_state;
    progress = ant.progress;
    current_edge = ant.current_edge;
    return *this;
}

Ant::~Ant() {
    if (current_Node != nullptr) {
        current_Node->remove_ant(this);
    }
    if (current_edge != nullptr) {
        current_edge->remove_ant(this);
    }
}

Node *Ant::get_current_node() const {
    return current_Node;
}

void Ant::set_current_node(Node *node) {
    if (current_Node != nullptr) {
        current_Node->remove_ant(this);
    }
    if (node != nullptr) {
        node->add_ant(this);
    }
    current_Node = node;
}

void Ant::apply_damages(uint8_t damages) {
    if (damages > etat.vie) {
        etat.vie = 0;
    } else {
        etat.vie -= damages;
    }
}

void Ant::kill() {
    etat.vie = 0;
}

void Ant::consume_water() {
    if (current_Node != nullptr && current_Node->get_type() == salle_type::EAU) {
        etat.eau = max_water;
    } else if (etat.eau > 0) {
        etat.eau--;
    }
    if (etat.eau == 0) {
        kill();
    }
}

bool Ant::alive() {
    return etat.vie != 0;
}

void Ant::move_along(Edge *edge) {
    action_state = AntActionState::MOVING;
    current_edge = edge;
    progress = 0;
}

void Ant::displace() {
    assert(action_state == AntActionState::MOVING);
    if (progress + EDGE_CROSS_SPEED < current_edge->get_length()) {
        if (current_Node != nullptr && progress == 0) {
            current_edge->add_ant(this);
            current_Node->remove_ant(this);
        }
        if (current_edge->can_be_crossed()) {
            progress += EDGE_CROSS_SPEED;
        }
    } else if (progress >= current_edge->get_length()) {
        current_edge->remove_ant(this);
        action_state = AntActionState::NONE;
    } else {
        current_Node = current_edge->get_other_node(current_Node);
        current_Node->add_ant(this);
        progress = current_edge->get_length();
    }
}

void Ant::begin_digging(Edge *edge) {
    assert(action_state == AntActionState::NONE);
    action_state = AntActionState::DIGGING;
    current_edge = edge;
}

void Ant::dig() {
    assert(action_state == AntActionState::DIGGING);
    current_edge->dig();
}

void Ant::stop_digging() {
    assert(action_state == AntActionState::DIGGING);
    action_state = AntActionState::NONE;
}

unsigned int Ant::gather_food() {
    assert(current_Node->get_type() == salle_type::NOURRITURE);
    unsigned int available_food = current_Node->gather_food(max_food - etat.nourriture);
    etat.nourriture += available_food;
    return available_food;
}

std::ostream &operator<<(std::ostream &os, const Ant &ant) {
    os << "======= Ant (" << ant.id << ") =======\n";
    os << "Team: " << ant.get_team_id() << "\n";
    os << "Status: ";
    switch (ant.get_action_state()) {
        case AntActionState::NONE:
            os << "Normal\n";
            break;
        case AntActionState::MOVING:
            os << "Moving along an edge\n";
            break;
        case AntActionState::DIGGING:
            os << "Digging\n";
            break;
    }
    if (ant.current_Node != nullptr) {
        os << "Current node: " << ant.current_Node->get_id() << "\n";
    }
    if (ant.current_edge != nullptr) {
        os << "Current edge: " << ant.current_edge->get_node1()->get_id() << " -- "
           << ant.current_edge->get_node2()->get_id() << "\n";
        os << "Progress: " << ant.get_progress() << "\n";
    }
    os << "== Stats: \n";
    os << "Health: " << ant.etat.vie << "/" << ant.queen->get_stat(Queen::Stat::LIFE) << "\n";
    os << "Food: " << ant.etat.nourriture << "/" << ant.max_food << "\n";
    os << "Water: " << ant.etat.eau << "/" << ant.max_water << "\n";
    os << "Attack: " << ant.get_attack() << "/" << ant.queen->get_stat(Queen::Stat::ATTACK) << "\n";
    os << "== Memory: \n";
    for (unsigned int i = 0; i < sizeof(ant.etat.memoire) / sizeof(ant.etat.memoire[0]); i++) {
        os << std::setfill('0') << std::setw(3) << (int)ant.etat.memoire[i];
        os << " ";
        if (i % 16 == 0) {
            os << "\n";
        }
    }
    return os;
}