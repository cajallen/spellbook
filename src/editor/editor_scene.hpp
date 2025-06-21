#pragma once

namespace spellbook {

struct Scene;

struct EditorScene {
    Scene*   p_scene = nullptr;
    
    virtual void setup() = 0 ;
    virtual void update();
    virtual void window(bool* p_open) { }
    virtual void shutdown() { }
};


struct EditorScenes {
    template<typename T>
    EditorScenes(T* t) {
        values().push_back(new T());
    }

    static vector<EditorScene*>& values() {
        static vector<EditorScene*> vec;
        return vec;
    }
};
#define ADD_EDITOR_SCENE(T) \
    EditorScenes _Add##T((T*) 0);
    

}