#include "algebra/Pipeline.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/Suboperator.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include <algorithm>

namespace inkfuse {

std::unique_ptr<Pipeline> Pipeline::repipeAll(size_t start, size_t end) const {
   // Find everything we provide that's not from strong output links.
   std::unordered_set<const IU*> out_provided;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
         if (!subop->outgoingStrongLinks()) {
            for (auto iu : subop->getIUs()) {
               if (!dynamic_cast<IR::Void*>(iu->type.get())) {
                  // We do not add void IUs as these present pseudo IUs to properly connect the graph.
                  out_provided.insert(iu);
               }
            }
         }
      });

   return repipe(start, end, out_provided);
}

std::unique_ptr<Pipeline> Pipeline::repipeRequired(size_t start, size_t end) const {
   // Find output IUs we provide.
   std::unordered_set<const IU*> out_provided;
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
         for (auto iu : subop->getIUs()) {
            if (!dynamic_cast<IR::Void*>(iu->type.get())) {
               // We do not add void IUs as these present pseudo IUs to properly connect the graph.
               out_provided.insert(iu);
            }
         }
      });

   // And figure out which of those will be needed by other sub-operators down the line.
   std::unordered_set<const IU*> out_required;
   std::for_each(
      suboperators.begin() + end,
      suboperators.end(),
      [&](const SuboperatorArc& subop) {
         for (auto iu : subop->getSourceIUs()) {
            if (out_provided.count(iu)) {
               out_required.insert(iu);
            }
         }
      });

   return repipe(start, end, out_required);
}

std::unique_ptr<Pipeline> Pipeline::repipe(size_t start, size_t end, const std::unordered_set<const IU*>& materialize) const {
   auto new_pipe = std::make_unique<Pipeline>();
   if (!supportsParallelCodegen()) {
      // New pipeline inherits the code generation properties of the old one.
      new_pipe->disallowParallelCodegen();
   }
   if (start == end) {
      // We are creating an empty pipe, return.
      return new_pipe;
   }

   const auto& first = suboperators[start];
   const auto& last = suboperators[end - 1];

   // Find out which IUs we need to access that are not provided by one of the suboperators in [start, end[.
   std::unordered_set<const IU*> in_provided;
   std::unordered_set<const IU*> in_required;
   // For repiping to work and be consistent, we order the inputs by order of appearance in the suboperator range.
   // This is really important during interpreted execution, as it allows us to get consistent input operator ordering.
   std::vector<const IU*> in_required_ordered;

   // Potential strong links.
   SuboperatorArc incoming_strong;
   SuboperatorArc outgoing_strong;

   // Analyze strong links on this sub-operator.
   if (first->incomingStrongLinks()) {
      // The suboperator has incoming strong links, meaning it cannot be separated from its input
      // suboperator. We have to attach the source of the strong link.
      // Note that at the moment, the length of a strong link chain can be at most one hop.
      const auto& input = graph.incoming_edges.at(first.get());
      assert(input.size() >= 1);
      auto strong_op = std::find_if(suboperators.begin(), suboperators.end(), [&](const SuboperatorArc& elem) {
         return elem.get() == input[0];
      });
      assert(strong_op != suboperators.end());
      incoming_strong = *strong_op;
      in_provided.insert((*strong_op)->getIUs()[0]);
      for (auto in_iu : incoming_strong->getSourceIUs()) {
         in_required.insert(in_iu);
         in_required_ordered.push_back(in_iu);
      }
   }

   if (last->outgoingStrongLinks()) {
      // The suboperator has outgoing strong links, meaning it cannot be separated from its consuming
      // suboperator. We have to move the end to contain all consumers.
      // Note that at the moment, the length of this chain can be at most one hop.
      const auto& output = graph.outgoing_edges.at(last.get());
      auto strong_op = std::find_if(suboperators.begin(), suboperators.end(), [&](const SuboperatorArc& elem) {
         return elem.get() == output[0];
      });
      assert(strong_op != suboperators.end());
      outgoing_strong = *strong_op;
   }

   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& subop) {
         for (auto iu : subop->getIUs()) {
            in_provided.insert(iu);
            assert(!in_required.count(iu));
         }
         for (auto iu : subop->getSourceIUs()) {
            if (!in_provided.count(iu) && !in_required.count(iu)) {
               // We don't want to read the same IU multiple times. If we did this there would
               // be multiple providers of the same IU which goes against IU SSA.
               in_required.insert(iu);
               in_required_ordered.push_back(iu);
            }
         }
      });

   if (const auto as_rtf = dynamic_cast<const RuntimeFunctionSubop*>(first.get());
       as_rtf && as_rtf->fname() == "materialize_tuple") {
      // HACKFIX - This is really terrible and breaks all the pretty abstractions we have built.
      // The TupleMaterializer at the moment has an arbitrary number of inputs. We would need
      // to properly separate inputs and codegen dependencies to make this clean :-( at the
      // moment it breaks the enumeration invariant. But that's an implementation detail, not
      // a conceptual limitation.

      assert(first->getSourceIUs().size() >= 1);
      in_required.clear();
      in_required_ordered.clear();
      in_required.insert(first->getSourceIUs()[0]);
      in_required_ordered.push_back(first->getSourceIUs()[0]);
   }

   if (!in_required.empty()) {
      // Build a FuseChunkSource for those that are not driven by an existing provider.
      // First we build the driver.
      auto& driver = new_pipe->attachSuboperator(FuseChunkSourceDriver::build());
      auto& driver_iu = **driver.getIUs().begin();
      // And the IU providers.
      for (auto iu : in_required_ordered) {
         if (!dynamic_cast<IR::Void*>(iu->type.get())) {
            // Ignore pseudo-IUs that are only used for proper codegen ordering.
            auto& provider = new_pipe->attachSuboperator(FuseChunkSourceIUProvider::build(driver_iu, *iu));
         }
      }
   }

   if (incoming_strong) {
      // An incoming strong link has to be attached after the base IU providers.
      new_pipe->attachSuboperator(incoming_strong);
   }

   // Copy all intermediate operators.
   std::for_each(
      suboperators.begin() + start,
      suboperators.begin() + end,
      [&](const SuboperatorArc& op) {
         new_pipe->attachSuboperator(op);
      });

   if (outgoing_strong) {
      // An outgoing strong link has to be attached before result materialization.
      new_pipe->attachSuboperator(outgoing_strong);
   }

   for (auto& out_iu : materialize) {
      // And build fuse chunk sinks for those.
      new_pipe->attachSuboperator(
         FuseChunkSink::build(nullptr, *out_iu));
   }

   return new_pipe;
}

const std::vector<Suboperator*>& Pipeline::getConsumers(Suboperator& subop) const {
   return graph.outgoing_edges.at(&subop);
}

const std::vector<Suboperator*>& Pipeline::getProducers(Suboperator& subop) const {
   return graph.incoming_edges.at(&subop);
}

Suboperator& Pipeline::getProvider(const IU& iu) const {
   return *iu_providers.at(&iu);
}

Suboperator* Pipeline::tryGetProvider(const IU& iu) const {
   if (iu_providers.count(&iu)) {
      return iu_providers.at(&iu);
   }
   return nullptr;
}

Suboperator& Pipeline::attachSuboperator(SuboperatorArc subop) {
   // Update the pipeline graph.
   for (const auto& depends : subop->getSourceIUs()) {
      // Resolve input.
      if (auto provider = tryGetProvider(*depends)) {
         // Add edges.
         graph.outgoing_edges[provider].push_back(subop.get());
         graph.incoming_edges[subop.get()].push_back(provider);
      }
   }
   // Register newly produced IUs within the current scope.
   for (auto iu : subop->getIUs()) {
      // Add the produced IU to the (potentially) new scope.
      iu_providers[iu] = subop.get();
   }
   suboperators.push_back(std::move(subop));
   return *suboperators.back();
}

const std::vector<SuboperatorArc>& Pipeline::getSubops() const {
   return suboperators;
}

void Pipeline::setPrettyPrinter(PrettyPrinter& printer_) {
   assert(!printer);
   printer = &printer_;
}

PrettyPrinter* Pipeline::getPrettyPrinter() {
   return printer;
}

TupleMaterializerState& PipelineDAG::attachTupleMaterializers(size_t discard_after, size_t tuple_size) {
   auto& inserted = runtime_state.emplace_back(
      discard_after,
      std::make_unique<TupleMaterializerState>(tuple_size));
   return static_cast<TupleMaterializerState&>(*inserted.second);
}

HashTableSimpleKeyState& PipelineDAG::attachHashTableSimpleKey(size_t discard_after, size_t key_size, size_t payload_size) {
   auto& inserted = runtime_state.emplace_back(discard_after, std::make_unique<HashTableSimpleKeyState>(key_size, payload_size));
   return static_cast<HashTableSimpleKeyState&>(*inserted.second);
}

HashTableComplexKeyState& PipelineDAG::attachHashTableComplexKey(size_t discard_after, uint16_t slots, size_t payload_size) {
   auto& inserted = runtime_state.emplace_back(discard_after, std::make_unique<HashTableComplexKeyState>(slots, payload_size));
   return static_cast<HashTableComplexKeyState&>(*inserted.second);
}

HashTableDirectLookupState& PipelineDAG::attachHashTableDirectLookup(size_t discard_after, size_t payload_size) {
   auto& inserted = runtime_state.emplace_back(discard_after, std::make_unique<HashTableDirectLookupState>(payload_size));
   return static_cast<HashTableDirectLookupState&>(*inserted.second);
}

Pipeline& PipelineDAG::getCurrentPipeline() const {
   assert(!pipelines.empty());
   return *pipelines.back();
}

Pipeline& PipelineDAG::buildNewPipeline() {
   for (auto& [subop_idx, pipe] : continuations) {
      // Attach the pipeline continuations.
      assert(!pipelines.empty());
      auto& curr_pipe = pipelines.back();
      auto& curr_subops = curr_pipe->getSubops();
      for (size_t k = subop_idx; k < curr_subops.size(); ++k) {
         // Attach all subops that were added after the continuation was crated.
         pipe->attachSuboperator(curr_subops[k]);
      }
      pipelines.push_back(std::move(pipe));
   }
   continuations.clear();
   pipelines.push_back(std::make_unique<Pipeline>());
   return *pipelines.back();
}

Pipeline& PipelineDAG::attachContinuation() {
   assert(!pipelines.empty());
   const size_t curr_idx = pipelines.back()->getSubops().size();
   auto new_pipe = std::make_unique<Pipeline>();

   // Both the current and the continuation pipeline cannot generate concurrently.
   // We added outer join support retrospectively and our suboperators have internal
   // state tied to a single compilation. We could support cloning suboperators, or detach
   // state in a clean way.
   new_pipe->disallowParallelCodegen();
   pipelines.back()->disallowParallelCodegen();

   continuations.push_back({curr_idx, std::move(new_pipe)});
   return *continuations.back().second;
}

void PipelineDAG::addRuntimeTask(PipelineDAG::RuntimeTask task) {
   runtime_tasks.push_back(std::move(task));
}

const std::vector<PipelineDAG::RuntimeTask>& PipelineDAG::getRuntimeTasks() const {
   return runtime_tasks;
}

const std::vector<PipelinePtr>& PipelineDAG::getPipelines() const {
   return pipelines;
}

}
