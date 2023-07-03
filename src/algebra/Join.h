#ifndef INKFUSE_JOIN_H
#define INKFUSE_JOIN_H

#include "algebra/RelAlgOp.h"
#include <list>
#include <optional>

namespace inkfuse {

enum class JoinType {
   Inner,
   LeftSemi,
};

/// A join in InkFuse. Joins produce very different suboperator DAGs depending on whether
/// they are primary key joins or not. This is because a PK join can be highly optimized:
/// - No need to keep lists of matching rows for a key within the hash table.
/// - No growing chunks - the output chunk will always be either the same size or smaller.
/// This means that we can create an optimzied suboperator layout for this type of join.
/// For non-PK joins we need to pack a much more complex join state and take care of potentially
/// growing chunks.
struct Join : public RelAlgOp {

   static std::unique_ptr<Join> build(
      std::vector<std::unique_ptr<RelAlgOp>> children_,
      std::string op_name_,
      std::vector<const IU*> keys_left_,
      std::vector<const IU*> payload_left_,
      std::vector<const IU*> keys_right_,
      std::vector<const IU*> payload_right_,
      JoinType type_,
      bool is_pk_join_);

   Join(
      std::vector<std::unique_ptr<RelAlgOp>> children_,
      std::string op_name_,
      std::vector<const IU*> keys_left_,
      std::vector<const IU*> payload_left_,
      std::vector<const IU*> keys_right_,
      std::vector<const IU*> payload_right_,
      JoinType type_,
      bool is_pk_join_);

   void decay(PipelineDAG& dag) const override;

   private:
   void plan();
   void decayPkJoin(PipelineDAG& dag) const;

   /// What join type is this?
   JoinType type;
   /// Is the left (build) side of the hash join a PK?
   bool is_pk_join;

   size_t key_size_left = 0;
   size_t payload_size_left = 0;
   size_t key_size_right = 0;
   size_t payload_size_right = 0;

   /// Packed scratch pad IU left.
   std::optional<IU> scratch_pad_left;
   /// Packed scratch pad IU right.
   std::optional<IU> scratch_pad_right;

   /// Lookup result left.
   std::optional<IU> lookup_left;
   /// Lookup result right.
   std::optional<IU> lookup_right;

   /// Void-typed pseudo-IUs that connect the key packing operators on
   /// the probe side with the hash table lookup.
   std::list<IU> right_pseudo_ius;
   /// Void-types pseudo-IU for the filter on rows that have no match.
   std::optional<IU> filter_pseudo_iu;

   /// Filtered build side in the probe phase. Char* typed.
   std::optional<IU> filtered_build;
   /// Filtered probe side in the probe phase. Byte[] typed.
   std::optional<IU> filtered_probe;

   /// The left input IUs.
   std::vector<const IU*> keys_left;
   std::vector<const IU*> payload_left;
   /// The left key/payload input IUs together.
   std::vector<const IU*> full_row_left;
   /// The right input IUs.
   std::vector<const IU*> keys_right;
   std::vector<const IU*> payload_right;

   /// The output IUs.
   std::list<IU> keys_left_out;
   std::list<IU> payload_left_out;
   std::list<IU> keys_right_out;
   std::list<IU> payload_right_out;
};

}

#endif //INKFUSE_JOIN_H
