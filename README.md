# Reflect

Forked from [this repo](https://github.com/LucaDavidian/Reflect), which is the code from [this blog post](http://www.lucadavidian.com/2021/06/21/simple-runtime-reflection-in-c/) by Luca Davidian.

## Modifications from upstream

 - Use `std::any` instead of `Any`
 - Option to pass a typeRegistry reference to the typeFactory

   This is useful in a plugin scenario, where both the plugin and the main application link statically against 
   a library which includes the Reflect headers. Both the application and the plugin dll/so will have distinct instances
   of the static variables, e.g. the typeRegistry. If a plugin is to define a reflected type and the main application
   uses the typeRegistry for lookup, it must pass its instance of the typeRegistry to the plugin.
   Otherwise the plugin will register the type to the local typeRegistry.
 - Removed "single include" because it became out of the sync after the before mentioned modifications.
