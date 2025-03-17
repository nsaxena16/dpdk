/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(C) 2025 Marvell International Ltd.
 */

#include <rte_graph_feature_arc_worker.h>

struct graph_interim_node_ctx {
	uint16_t last_index;
};

#define FEATURE_INTERIM_NODE_LAST_NEXT_INDEX(ctx) \
	(((struct graph_interim_node_ctx *)ctx)->last_index)

static int
feature_arc_interim_node_init(const struct rte_graph *graph, struct rte_node *node)
{

	RTE_SET_USED(graph);

	/* original index is at 0th index*/
	FEATURE_INTERIM_NODE_LAST_NEXT_INDEX(node->ctx) = 0;

	return 0;
}

static __rte_always_inline int16_t
feature_arc_node_process(struct rte_graph *graph, struct rte_node *node,
			 void **objs, uint16_t nb_objs,
			 const int is_start_node,
			 const int is_feature_enabled)
{
	struct rte_graph_feature_arc *arc =
		(struct rte_graph_feature_arc *)node->feature_arc_ptr;
	int feat_dyn_off = rte_graph_feature_arc_mbuf_dynfield_offset_get();
	struct rte_graph_feature_arc_mbuf_dynfields *mbfields = NULL;
	void **to_next, **from;
	uint16_t last_spec = 0;
	rte_edge_t next_index;
	struct rte_mbuf *mbuf;
	uint16_t held = 0;
	uint16_t next;
	int is_feat;
	int i;

	RTE_SET_USED(is_start_node);

	if (!is_feature_enabled) {
		to_next = rte_node_next_stream_get(graph, node,
						   0 /* original next_node is at index 0*/,
						   nb_objs);
		rte_node_next_stream_move(graph, node, 0);
		return nb_objs;
	}

	/* Speculative next */
	next_index = FEATURE_INTERIM_NODE_LAST_NEXT_INDEX(node->ctx);

	from = objs;
	to_next = rte_node_next_stream_get(graph, node, next_index, nb_objs);
	for (i = 0; i < nb_objs; i++) {

		mbuf = (struct rte_mbuf *)objs[i];

		is_feat = 0;
		next = 0;

		/* Send mbuf to next enabled feature */
		mbfields = rte_graph_feature_arc_mbuf_dynfields_get(mbuf, feat_dyn_off);

		if (is_start_node)
			is_feat = rte_graph_feature_data_first_feature_get(arc, mbuf->port,
									   &mbfields->feature_data,
									   &next);
		else
			is_feat = rte_graph_feature_data_next_feature_get(arc,
									  &mbfields->feature_data,
									  &next);

		/* adjust next node only when feature is enabled, else next node is 0*/
		is_feat = is_feat * node->base_arc_next_edge;
		next -= is_feat;

		if (unlikely(next_index != next)) {
			/* Copy things successfully speculated till now */
			rte_memcpy(to_next, from, last_spec * sizeof(from[0]));
			from += last_spec;
			to_next += last_spec;
			held += last_spec;
			last_spec = 0;

			rte_node_enqueue_x1(graph, node, next, from[0]);
			from += 1;
		} else {
			last_spec += 1;
		}
	}
	/* !!! Home run !!! */
	if (likely(last_spec == nb_objs)) {
		rte_node_next_stream_move(graph, node, next_index);
		return nb_objs;
	}
	held += last_spec;
	rte_memcpy(to_next, from, last_spec * sizeof(from[0]));
	rte_node_next_stream_put(graph, node, next_index, held);

	FEATURE_INTERIM_NODE_LAST_NEXT_INDEX(node->ctx) = next;

	return nb_objs;
}
