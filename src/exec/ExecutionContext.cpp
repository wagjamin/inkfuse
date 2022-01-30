#include "exec/ExecutionContext.h"

namespace inkfuse {

    void Scope::registerProducer(IURef iu, Suboperator &op)
    {
        iu_producers[&iu.get()] = &op;
    }

    Suboperator* Scope::getProducer(IURef iu)
    {
        auto it = iu_producers.find(&iu.get());
        return it->second;
    }

}
