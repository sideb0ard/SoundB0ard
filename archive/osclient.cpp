#include <lo/lo.h>
#include <lo/lo_cpp.h>

#include <iostream>

int main() {
  lo::Address a("localhost", "9000");
  lo::Address ta("localhost", "12345");

  /*
   * An individual message
   */
  a.send("pitch", "i", 7890987);
  ta.send("pitch", "i", 7890987);

  /*
   * Initalizer lists and message constructors are supported, so
   * that bundles can be created easily:
   */
  a.send(lo::Bundle({{"pitch", lo::Message("i", 1234321)},
                     {"pitch", lo::Message("i", 4321234)}}));

  ta.send(lo::Bundle({{"pitch", lo::Message("i", 1234321)},
                      {"pitch", lo::Message("i", 4321234)}}));
  /*
   * Polymorphic overloads on lo::Message::add() mean you don't need
   * to specify the type explicitly.  This is intended to be useful
   * for templates.
   */
  lo::Message m;
  m.add(7654321);
  a.send("pitch", m);
  ta.send("pitch", m);
}
