//#include "nodes.hpp"
//
//#include "registers.hpp"
//
//using namespace comp;
//
//void root::generate(std::ostream &out) const {
//    out << "[bits 64]" << '\n';
//    out << "global _start" << '\n';
//
//    out << "extern puts" << '\n';
//    out << "extern exit" << '\n';
//
//    out << "section .text" << '\n';
//
//    for (const auto &node : text_nodes) {
//        node->generate(out);
//    }
//
//    out << "section .data" << '\n';
//
//    for (const auto &node : data_nodes) {
//        node->generate(out);
//    }
//}
//
//void global_string::generate(std::ostream &out) const {
//    out << this->name << " db \"" << value << "\", 0" << '\n';
//}
//
//void global::function::generate(std::ostream &out) const {
//    out << "    " << name << ":" << '\n';
//
//    block.generate(out);
//}
//
//void block::generate(std::ostream &out) const {
//    out << "        push    rbp" << '\n';
//    out << "        mov     rbp, rsp" << '\n';
//
//    for (const auto &instruction : instructions) {
//        instruction->generate(out);
//    }
//
//    out << "        pop     rbp" << '\n';
//    out << "        ret" << '\n';
//}
//
//void block::call::generate(std::ostream &out) const {
//    for (auto i = 0; i < args.size(); i++) {
//        const char* param_register = comp::registers::param_register_string(i);
//
//        out << "        push    " << param_register << '\n';
//        out << "        mov     " << param_register << ", " << args[i]->get_reference() << '\n';
//    }
//
//    out << "        call    " << name << '\n';
//
//    for (auto i = 0; i < args.size(); i++) {
//        out << "        pop     " << comp::registers::param_register_string(i) << '\n';
//    }
//}