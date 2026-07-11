# Copyright
Copyright (C) 2026 Charles Nitschke montyn09@gmail.com All Rights Reserved

$ git submodule add git@github.com:libsdl-org/SDL.git
$ git submodule add git@github.com:g-truc/glm.git

# Libraries
To add a package, atleast for now, add a submodule into vendor for the
git repo:
$ git submodule add git@github.com:g-truc/glm.git

Then point CMake at it by adding the two lines:
add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)