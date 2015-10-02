//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#include <cassert>
#include <cstdio>


#include "../../regression/common/hbr_utils.h"
#include "../../regression/common/far_utils.h"
#include "../../regression/common/cmp_utils.h"

#include "init_shapes.h"

//
// Regression testing matching Far to Hbr (default CPU implementation)
//
// Notes:
// - precision is currently held at 1e-6
//
// - results cannot be bitwise identical as some vertex interpolations
//   are not happening in the same order.
//
// - only vertex interpolation is being tested at the moment.
//
#define PRECISION 1e-6

static bool g_debugmode = false;

//------------------------------------------------------------------------------
// Vertex container implementation.
//
struct Vertex {

    Vertex() : _index(0) { }
    Vertex(int index) : _index(index) { }

    // Minimal required interface ----------------------
    // ---------------------------------------------
    Vertex operator[](int index) const {
        std::cout << "Index: " << _index+index << "\n";
        return Vertex(_index+index);
    }

    void Clear(void * = 0) {
        std::cout << "-- CLEAR -- \n";
    }

    void AddWithWeight(Vertex const & src, float weight) {
        std::cout << "    [" << _index << "] += [" << src._index << "] * " << weight << "\n"; 
    }

    // Public interface ------------------------------------
    // ---------------------------------------------

    void SetIndex(int index) {
        _index = index;
    }

private:
    int _index;
};


//------------------------------------------------------------------------------
typedef OpenSubdiv::Far::TopologyRefiner               FarTopologyRefiner;
typedef OpenSubdiv::Far::TopologyRefinerFactory<Shape> FarTopologyRefinerFactory;


int
interpolate(ShapeDesc const & desc, int maxlevel) {
    typedef Vertex T;

    // Far interpolation
    Shape * shape = Shape::parseObj(desc.data.c_str(), desc.scheme);

    FarTopologyRefiner * refiner =
        FarTopologyRefinerFactory::Create(*shape,
            FarTopologyRefinerFactory::Options(
                GetSdcType(*shape), GetSdcOptions(*shape)));
    assert(refiner);

    FarTopologyRefiner::UniformOptions options(maxlevel);
    options.fullTopologyInLastLevel=true;
    refiner->RefineUniform(options);

    T srcVerts(0);
    T dstVerts(refiner->GetLevel(0).GetNumVertices());
    OpenSubdiv::Far::PrimvarRefiner primvarRefiner(*refiner);

    for (int i = 1; i <= refiner->GetMaxLevel(); ++i) {
        std::cout << "\n" 
            << "-----------------------------------------\n"
            << "LEVEL " << i << ": " << desc.name << "\n"
            << "-----------------------------------------\n";
        primvarRefiner.Interpolate(i, srcVerts, dstVerts);
        srcVerts = dstVerts;
        dstVerts = dstVerts[refiner->GetLevel(i).GetNumVertices()];
    }

    delete shape;
    delete refiner;

    return 1; // success
}

//------------------------------------------------------------------------------
int main(int /* argc */, char ** /* argv */) {

    int levels=1, total=0;

    initShapes();

    if (g_debugmode)
        printf("[ ");
    else
        printf("precision : %f\n",PRECISION);
    for (int i=0; i<(int)g_shapes.size(); ++i) {
        total+=interpolate(g_shapes[i], levels);
    }

    if (g_debugmode)
        printf("]\n");
    else {
        if (total==0)
          printf("All tests passed.\n");
        else
          printf("Total failures : %d\n", total);
    }
}

//------------------------------------------------------------------------------
