#pragma once
#include "isosat.hpp"
#include "ikpxtree.hpp"
#include "../cqueue/blockingconcurrentqueue.h"

#define COMMAND_TERMINATE 2

struct workitem {

    u64seq initial_rows;
    int16_t exhausted_width;
    int16_t maximum_width;
    int16_t lookahead;
    int16_t direction;

};

typedef moodycamel::BlockingConcurrentQueue<workitem> WorkQueue;

void worker_loop(WorkQueue *from_master, WorkQueue *to_master, Velocity vel, std::vector<int> prime_implicants) {

    while (true) {

        workitem item;
        from_master->wait_dequeue(item);

        if (item.direction == COMMAND_TERMINATE) { break; }

        MetaProblem mp(item.initial_rows, vel);

        mp.find_all_solutions(item.maximum_width, item.exhausted_width,
            prime_implicants, item.lookahead, [&](const u64seq &svec) {

            workitem item2;

            item2.initial_rows = svec;
            item2.exhausted_width = item.maximum_width;
            item2.maximum_width = 0;
            item2.lookahead = 0;
            item2.direction = item.direction;

            to_master->enqueue(item2);

        });

        // Inform the master that the work has been completed:
        to_master->enqueue(item);
    }

}


struct semisearch {

    Velocity vel;
    ikpxtree tree;
    int direction;
    lab32_t *lab;
    int search_width;
    int lookahead;
    int jumpahead;

    int items_in_aether;
    uint32_t record_depth;
    std::vector<int> truth_table;
    std::vector<int> prime_implicants;
    WorkQueue from_master;
    std::vector<std::thread> workers;

    semisearch(const Velocity &vel, int direction, lab32_t *lab, int search_width, int lookahead, int jumpahead) :
        vel(vel), tree(vel.vradius() * 2), direction(direction), lab(lab),
        search_width(search_width), lookahead(lookahead), jumpahead(jumpahead) {

        search_width = 2;
        std::string rule = apg::get_all_rules()[0];
        truth_table = truth_table_for_rule(lab, rule);
        prime_implicants = truth_table_to_prime_implicants(truth_table);

        items_in_aether = 0;
        record_depth = 0;
    }

    void enqueue(const u64seq &elem, int exhausted_width) {

        if (exhausted_width >= search_width) { return; }

        workitem item;

        item.initial_rows = elem;
        item.exhausted_width = exhausted_width;
        item.maximum_width = search_width;
        item.lookahead = lookahead;
        item.direction = direction;

        from_master.enqueue(item);
        items_in_aether += 1;
    }

    u64seq inject(const uint64_t *fullseq) {

        auto elem = tree.inject(fullseq);
        if (elem.empty()) { return elem; }

        auto it = tree.preds.find(elem);

        if (it->second.exhausted_width == 0) {
            // not previously in map
            it->second.exhausted_width = 1;
            enqueue(elem, 1);
        }

        return elem;
    }

    void load_file(std::string filename) {

        apg::pattern robin(lab, filename);
        std::vector<uint64_t> results;
        int n7 = ltransform(robin, vel, results);

        for (uint64_t i = 0; i < results.size(); i += n7) {
            inject(&(results[i]));
        }
    }

    void inject_partial(const u64seq &results) {

        u64seq p;

        for (int i = 0; i < jumpahead; i++) {
            auto pc = inject(&(results[i]));
            if (!(pc.empty())) { p = pc; }
        }

        if (p.empty()) { return; }

        if (tree.preds[p].depth > record_depth) {
            record_depth = tree.preds[p].depth;
            auto pat = tree.materialise(lab, p.data());
            std::cout << "\n#C depth = " << record_depth << std::endl;
            ikpx2golly(pat, vel).write_rle(std::cout);
        }
    }

    void launch_thread(WorkQueue &to_master, int iters=1) {
        for (int i = 0; i < iters; i++) {
            workers.emplace_back(worker_loop, &from_master, &to_master, vel, prime_implicants);
        }
    }

    void join_threads() {

        workitem item2;
        item2.exhausted_width = 0;
        item2.maximum_width = 0;
        item2.lookahead = 0;
        item2.direction = COMMAND_TERMINATE;

        for (size_t i = 0; i < workers.size(); i++) {
            from_master.enqueue(item2);
        }

        for (size_t i = 0; i < workers.size(); i++) {
            workers[i].join();
        }

        workers.clear();
    }

};

void master_loop(semisearch &searcher, WorkQueue &to_master) {

    uint64_t xcount = 0;

    do {

        workitem item;
        to_master.wait_dequeue(item);

        if (item.lookahead) {
            searcher.tree.preds[item.initial_rows].exhausted_width = item.maximum_width;
            searcher.items_in_aether -= 1;
        } else {
            searcher.inject_partial(item.initial_rows);
        }

        xcount += 1;

        if ((xcount & 255) == 0) {
            std::cout << xcount << " iterations completed; qsize = " << searcher.items_in_aether;
            std::cout << "; treesize = " << searcher.tree.preds.size() << std::endl;
        }

    } while (searcher.items_in_aether);

}

int run_ikpx(const std::vector<std::string> &arguments) {

    std::cerr << "sizeof(workitem) = " << sizeof(workitem) << std::endl;

    std::string velocity = "";
    std::string directory = "";

    for (size_t i = 0; i < arguments.size(); i++) {

        std::string command = arguments[i];

        if ((command == "-v") || (command == "--velocity")) {
            velocity = arguments[++i];
        } else if ((command == "-d") || (command == "--directory")) {
            directory = arguments[++i];
        } else {
            ERREXIT("unknown command: " << command);
        }
    }

    if (velocity == "") {
        ERREXIT("velocity must be specified.");
    }

    Velocity vel(velocity);

    std::cerr << "Valid velocity: \033[32;1m(" << vel.vd << "," << vel.hd << ")c/" << vel.p << "\033[0m" << std::endl;

    apg::lifetree<uint32_t, 1> lt(100);

    WorkQueue to_master;

    semisearch hs(vel, 0, &lt, 24, 30, 15);
    hs.load_file("docs/almost.rle");
    hs.launch_thread(to_master, 8);

    master_loop(hs, to_master);

    hs.join_threads();

    return 0;

}

