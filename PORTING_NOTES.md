# Geometrize2GD: port to Geode 6.0.0 / GD 2.2091

## Changed
- mod.json: geode 6.0.0, GD 2.2091 (win/android/mac/ios), version v1.3.0
- CMakeLists.txt: project version 1.3.0
- src/layers/ImportPopup.h / .cpp: rewritten for the Geode 6 API
  - Popup<CCArray*> + initAnchored -> plain geode::Popup, init(w, h), setup done after init
  - TextInput getInputNode()/setDelegate/textChanged -> setCallback() lambdas, setTextColor() for errors
  - EventListener<Task<...>> + file::pick -> async::spawn(file::pick(...), callback)
  - matjson used via asDouble().unwrapOr(...), isObject/contains/asArray
  - Only valid shapes (known type, score > 0) are placed. Previously placement ignored the type filter.
  - Colors read as R, G, B (alpha ignored, as before)

## Not verified
- This was not compiled. Geode mods build for Windows/macOS/iOS/Android, not Linux.
- matjson accessor names (asDouble, asArray, contains, isObject) follow Geode 6 usage, but check them against the matjson version bundled with Geode 6.
- flipObjectsY is bound only for win and ios in the 2.2091 bindings, so macOS may need a binding address.
- The picker callback captures `this`. If the popup closes before the user picks a file, that could crash. Consider a safer lifetime check.
