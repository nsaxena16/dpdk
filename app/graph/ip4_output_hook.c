/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(C) 2025 Marvell International Ltd.
 */

#include <rte_graph.h>
#include <rte_graph_worker.h>
#include <rte_graph_feature_arc_worker.h>

#include "rte_node_ip4_api.h"

#define IP4_OUTPUT_HOOK_FEATURE1_NAME "app_ip4_out_feat1"
#define IP4_OUTPUT_HOOK_FEATURE2_NAME "app_ip4_out_feat2"

struct output_hook_node_ctx {
	rte_graph_feature_arc_t out_arc;
	uint16_t last_index;
};

#define OUTPUT_HOOK_FEATURE_ARC(ctx) \
	(((struct output_hook_node_ctx *)ctx)->out_arc)

#define OUTPUT_HOOK_LAST_NEXT_INDEX(ctx) \
	(((struct output_hook_node_ctx *)ctx)->last_index)

static int
__app_graph_ip4_output_hook_node_init(const struct rte_graph *graph, struct rte_node *node)
{
	rte_graph_feature_arc_t feature;

	RTE_SET_USED(graph);

	rte_graph_feature_arc_lookup_by_name(RTE_IP4_OUTPUT_FEATURE_ARC_NAME, &feature);

	OUTPUT_HOOK_FEATURE_ARC(node->ctx) = feature;
	/* pkt_drop */
	OUTPUT_HOOK_LAST_NEXT_INDEX(node->ctx) = 0;

	return 0;
}

static __rte_always_inline uint16_t
__app_graph_ip4_output_hook_node_process(struct rte_graph *graph, struct rte_node *node,
					 void **objs, uint16_t nb_objs)
{
	RTE_SET_USED(objs);

	rte_node_next_stream_get(graph, node,
				 0 /* original next_node is at index 0*/,
				 nb_objs);
	rte_node_next_stream_move(graph, node, 0);

	return nb_objs;
}

static int
app_graph_ip4_output_hook_node1_init(const struct rte_graph *graph, struct rte_node *node)
{
	return __app_graph_ip4_output_hook_node_init(graph, node);
}

static __rte_always_inline uint16_t
app_graph_ip4_output_hook_node1_process(struct rte_graph *graph, struct rte_node *node,
					void **objs, uint16_t nb_objs)
{
	return __app_graph_ip4_output_hook_node_process(graph, node, objs, nb_objs);
}

static struct rte_node_register app_graph_ip4_output_hook_node1 = {
	.process = app_graph_ip4_output_hook_node1_process,
	.init = app_graph_ip4_output_hook_node1_init,
	.name = "app_ip4_output_node1",
	.nb_edges = 2,
	.next_nodes = {
		[0] = "pkt_drop",
		[1] = "pkt_cls",
	},
};

RTE_NODE_REGISTER(app_graph_ip4_output_hook_node1);

static int
app_graph_ip4_output_hook_node2_init(const struct rte_graph *graph, struct rte_node *node)
{
	return __app_graph_ip4_output_hook_node_init(graph, node);
}

static __rte_always_inline uint16_t
app_graph_ip4_output_hook_node2_process(struct rte_graph *graph, struct rte_node *node,
					void **objs, uint16_t nb_objs)
{
	return __app_graph_ip4_output_hook_node_process(graph, node, objs, nb_objs);
}

static struct rte_node_register app_graph_ip4_output_hook_node2 = {
	.process = app_graph_ip4_output_hook_node2_process,
	.init = app_graph_ip4_output_hook_node2_init,
	.name = "app_ip4_output_node2",
	.nb_edges = 1,
	.next_nodes = {
		[0] = "pkt_drop",
	},
};

RTE_NODE_REGISTER(app_graph_ip4_output_hook_node2);

/* if feature1 */
struct rte_graph_feature_register app_graph_ip4_output_hook_feature1 = {
	.feature_name = IP4_OUTPUT_HOOK_FEATURE1_NAME,
	.arc_name = RTE_IP4_OUTPUT_FEATURE_ARC_NAME,
	/* Same as regular function */
	.feature_process_fn = app_graph_ip4_output_hook_node1_process,
	.feature_node = &app_graph_ip4_output_hook_node1,
	.runs_before =  IP4_OUTPUT_HOOK_FEATURE2_NAME,
};

/* if feature2 (same as f1) */
struct rte_graph_feature_register app_graph_ip4_output_hook_feature2 = {
	.feature_name = IP4_OUTPUT_HOOK_FEATURE2_NAME,
	.arc_name = RTE_IP4_OUTPUT_FEATURE_ARC_NAME,
	/* Same as regular function */
	.feature_node = &app_graph_ip4_output_hook_node2,
	.feature_process_fn = app_graph_ip4_output_hook_node2_process,
};

RTE_GRAPH_FEATURE_REGISTER(app_graph_ip4_output_hook_feature1);
RTE_GRAPH_FEATURE_REGISTER(app_graph_ip4_output_hook_feature2);
