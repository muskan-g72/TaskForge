#pragma once

namespace taskforge {

enum class WorkerState {
    Starting,
    Idle,
    Running,
    Stopping,
    Stopped,
    Crashed
};

}  