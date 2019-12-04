//--------------------------------------------------------------------------------
//
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met :
//
//	*Redistributions of source code must retain the above copyright notice, this
//	list of conditions and the following disclaimer.
//
//	* Redistributions in binary form must reproduce the above copyright notice,
//	this list of conditions and the following disclaimer in the documentation
//	and/or other materials provided with the distribution.
//	
//	* Neither the name of the copyright holder nor the names of its
//	contributors may be used to endorse or promote products derived from
//	this software without specific prior written permission.
//	
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//	DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//	OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Copyright(c) 2019, Sergen Eren
// All rights reserved.
//----------------------------------------------------------------------------------
// 
//	Version 1.0: Sergen Eren, 29/11/2019
//
// File: This is the implementation file for BVH_Builder class functions 
//
//-----------------------------------------------

#include "bvh_builder.h"
#include <iostream>

extern "C" void BuildBVH(BVH& bvh, GPU_VDB* volumes, int numVolumes, AABB &sceneBounds, bool debug_bvh);
extern "C" void build_octree(OCTNode *root, GPU_VDB *volumes, int num_volumes, int depth, bool debug);

// Build the BVH that will be sent to the render kernel
bvh_error_t BVH_Builder::build_bvh(std::vector<GPU_VDB> vdbs, int num_volumes, AABB &sceneBounds) {
	

	// Build BVH
	volumes = new GPU_VDB[num_volumes];
	
	checkCudaErrors(cudaMalloc(&volumes, num_volumes * sizeof(GPU_VDB)));
	checkCudaErrors(cudaMemcpy(volumes, vdbs.data(), num_volumes * sizeof(GPU_VDB), cudaMemcpyHostToDevice));

	BuildBVH(bvh, volumes, num_volumes, sceneBounds, m_debug_bvh);
	
	

	// Build octree 
	octree.root_node = new OCTNode;
	// <3, 3, 3, 3,> Octree
	octree.root_node->depth = 4;

	for (int i = 0; i < num_volumes; ++i) {

		octree.root_node->bbox.pmax = fmaxf(octree.root_node->bbox.pmax, vdbs.at(i).Bounds().pmax);
		octree.root_node->bbox.pmin = fminf(octree.root_node->bbox.pmin, vdbs.at(i).Bounds().pmin);
		octree.root_node->vol_indices[i] = i;
	}
	for (int y = 0; y < num_volumes; ++y) {
		if (Overlaps(octree.root_node->bbox, vdbs.at(y).Bounds())) {
			octree.root_node->num_volumes++;
		}
	}
	std::cout << "num volumes for root is " << octree.root_node->num_volumes << "\n";

	printf("Root node bounds \npmin.x: %f, pmin.y: %f, pmin.z: %f \npmax.x: %f, pmax.y: %f, pmax.z: %f\n",
		octree.root_node->bbox.pmin.x, octree.root_node->bbox.pmin.y, octree.root_node->bbox.pmin.z,
		octree.root_node->bbox.pmax.x, octree.root_node->bbox.pmax.y, octree.root_node->bbox.pmax.z);

	octree.m_debug = false; // make this true to debug octree nodes 
	
	checkCudaErrors(cudaMalloc(&root, sizeof(OCTNode)));
	checkCudaErrors(cudaMemcpy(root, octree.root_node, sizeof(OCTNode), cudaMemcpyHostToDevice));
	
	build_octree(root, volumes, vdbs.size() , /*octree depth*/ octree.root_node->depth-1 , octree.m_debug);
	
	//octree.create_tree(vdbs, octree.root_node, 3);

	cudaFree(volumes);
	volumes = nullptr;

	return BVH_NO_ERR;
	
}