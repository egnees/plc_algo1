#include "TaskSolver.h"
#include "../algo/dp.h"

class dpTaskSolver : public TaskSolver {
public:
    Params get_params() override;
    Params estimate() override;
    Params solve() override;

    void init(const py::str& input_path,
              const py::str& output_path,
              const py::kwargs& kwargs) override;

private:
    const int DEFAULT_STEP_X{70};
    int step_x;

    int n;

    int get_lcm() const;
    std::pair<mut_t, pin_add_t> get_input(int LCM) const;
};