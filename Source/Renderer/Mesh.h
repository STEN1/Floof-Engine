#include "../Floof.h"
#include "Vertex.h"

namespace FLOOF {
    template<typename VertexType>
    class StaticMesh {
    public:
        StaticMesh();
    };

    template<typename VertexType>
    class DynamicMesh {
    public:
        DynamicMesh();
    };

    using ModelMesh = StaticMesh<MeshVertex>;
    using StaticLineMeshComponent = StaticMesh<ColorVertex>;
    using DynamicLineMeshComponent = DynamicMesh<ColorVertex>;
}