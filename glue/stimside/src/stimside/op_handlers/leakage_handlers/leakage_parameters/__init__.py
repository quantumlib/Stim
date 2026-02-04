from stimside.op_handlers.leakage_handlers.leakage_parameters.leakage_controlled_error import (
    LeakageControlledErrorParams,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters.leakage_transition_1 import (
    LeakageTransition1Params,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters.leakage_transition_z import (
    LeakageTransitionZParams,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters.leakage_transition_2 import (
    LeakageTransition2Params,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters.leakage_conditioning import (
    LeakageConditioningParams,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters.leakage_measurement import (
    LeakageMeasurementParams,
)

# poor man's discriminated union
LeakageParams = (
    LeakageTransition1Params
    | LeakageControlledErrorParams
    | LeakageTransitionZParams
    | LeakageTransition2Params
    | LeakageConditioningParams
    | LeakageMeasurementParams
)
