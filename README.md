# MDViewModel

The MDViewModel plugin is an Unreal-ified implementation of the [Model-View-ViewModel pattern](https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93viewmodel), supporting UMG Widgets and Actor Blueprints as Views with the goal of automating the most common binding patterns.

- Bind UMG Widget, Actor and Object Blueprints to View Models
- Caches View Model objects for the life time of their model data source
- Automatically fetch/create View Models and bind them to Views based on tuning set in each View
- Generates Blueprint events automatically for binding view model data to Blueprint logic, facilitating event-based updates to your Views
- Custom editor extensions to improve view model workflows and debugging
- FieldNotify properties and functions on view models are automatically exposed as Blueprint Events
- Integrates with the Blueprint Editor Debugger to display the values of properties on view models that are set on the instance being debugged
- Create View Models as C++ classes or Blueprints

## Compatibility

MDViewModel supports C++ projects using Unreal Engine 5.1 or greater. Features may vary depending on the engine version.

## Philosophy

This plugin is written with the assumption that you follow certain methodologies when using view models.

- Visual properties and logic are set within the View's Blueprint.
- A view never communicates directly with the model object.
  - Everything goes through view models.
- Model objects (gameplay objects, online backend systems, etc) are not aware of view models.
  - Instead view models reach into the external systems to grab the data they need, bind delegates and send out commands.
- A view model will only represent a single model object (a view model can hold sub-view models though) and will only ever represent that 1 object.
  - Each model object should have a unique view model.
  - For a view to display data from a different object, it should bind to a that object's view model.

MDViewModel does what it can to streamline the most common workflows and patterns for binding to model data, but some more complex patterns will require more manual work.

While this is how MDViewModel is intended to be used, it's powerful enough to be used (or abused) for other patterns as well.

## Getting Started

Check out the [Getting Started page](https://github.com/DoubleDeez/MDViewModel/wiki/Getting-Started) to get set up and learn how to create and bind to a view model.

## Future Features

- Integration with UMG's Preview Mode for quick iteration
- Support caching view models relative to TSharedPtrs

https://github.com/DoubleDeez/MDViewModel/assets/1893819/59474726-c86b-42c8-8fce-4a7af77a8931
