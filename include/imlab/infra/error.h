// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#ifndef INCLUDE_IMLAB_INFRA_ERROR_H_
#define INCLUDE_IMLAB_INFRA_ERROR_H_
//---------------------------------------------------------------------------
#include <stdexcept>
#include <string>
//---------------------------------------------------------------------------
namespace imlab {
//---------------------------------------------------------------------------
struct TransactionRollback : std::runtime_error {
   // Constructor
   explicit TransactionRollback(const char* what)
      : std::runtime_error(what) {}
};
//---------------------------------------------------------------------------
struct SchemaCompilationError : std::exception {
   // Constructor
   explicit SchemaCompilationError(const char* what) : message_(what) {}
   // Constructor
   explicit SchemaCompilationError(const std::string& what) : message_(what) {}
   // Destructor
   virtual ~SchemaCompilationError() throw() {}
   // Get error message
   virtual const char* what() const throw() { return message_.c_str(); }

   protected:
   // Error message
   std::string message_;
};
//---------------------------------------------------------------------------
} // namespace imlab
//---------------------------------------------------------------------------
#endif // INCLUDE_IMLAB_INFRA_ERROR_H_
//---------------------------------------------------------------------------
