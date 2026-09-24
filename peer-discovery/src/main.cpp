#include "../include/common.h"
#include "../include/PeerDiscovery.h"

int main(int argc, char** argv) {
    Config cfg{};
    if (!parse_args(argc, argv, cfg))
        return 1;

    PeerDiscovery app(cfg);
    return app.running();
}