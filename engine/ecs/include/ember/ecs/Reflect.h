#pragma once
//
// Compile-time field reflection.
//
// A "visitor" is any object exposing operator()(const char* name, FieldT& value)
// overloads for the supported field types. The serializer supplies a visitor that
// reads/writes YAML; a future inspector supplies one that draws UI. No RTTI.
//
//   EMBER_REFLECT(Transform) {
//       EMBER_FIELD(position);
//       EMBER_FIELD(rotation);
//       EMBER_FIELD(scale);
//   }
//
namespace ember {

// Primary template: declared, never defined. Each component provides an overload
// via EMBER_REFLECT. A call on an unreflected type is a compile/link error.
template<typename T, typename Visitor>
void reflect(T& obj, Visitor& visit);

} // namespace ember

#define EMBER_REFLECT(Type) \
    template<typename Visitor> void reflect(Type& obj, Visitor& visit)

#define EMBER_FIELD(member) visit(#member, obj.member)
