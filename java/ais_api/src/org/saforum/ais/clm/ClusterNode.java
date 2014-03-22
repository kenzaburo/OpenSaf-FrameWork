/*******************************************************************************
**
** SPECIFICATION VERSION:
**   SAIM-AIS-R2-JD-A.01.01
**   SAI-Overview-B.01.01
**   SAI-AIS-CLM-B.01.01
**
** DATE:
**   Wed Aug 6 2008
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS.
**
** Copyright 2008 by the Service Availability Forum. All rights reserved.
**
** Permission to use, copy, and distribute this mapping specification for any
** purpose without fee is hereby granted, provided that this entire notice
** is included in all copies. No permission is granted for, and users are
** prohibited from, modifying or making derivative works of the mapping
** specification.
**
*******************************************************************************/

package org.saforum.ais.clm;

/**
 * This class contains information about a cluster node.
 * <P><B>SAF Reference:</B> <code>SaClmClusterNodeT</code>
 * @version CLM-B.01.01 (SAIM-AIS-R2-JD-A.01.01)
 * @since CLM-B.01.01
 *
 */
public class ClusterNode {

    /**
     * The constant LOCAL_NODE_ID, used as nodeId in an API method of the
     * Cluster Membership Service, identifies the nodeId of the cluster node
     * that hosts the invoking process.
     */
    public final static int LOCAL_NODE_ID = 0xFFFFFFFF;

    /**
     * Node identifier that is unique in the cluster. A given nodeId identifies
     * a node only for as long as the node is a member of the cluster
     * membership. A node, identified by its name, can join the cluster again
     * with the same nodeId that it had when it left the cluster membership or
     * with a different one.
     */
    public int nodeId;

    /**
     * Node network communication address.
     */
    public NodeAddress nodeAddress;

    /**
     * Node name that is unique in the cluster. Node names are not attached to a
     * particular piece of hardware or a particular physical location of that
     * hardware inside the cluster but rather to the ability (from a
     * provisioning and I/O connectivity point of view) to host a particular set
     * of components. Hence, replacing a board by another similar board, moving
     * a board to another location in a chassis, booting an upward compatible
     * version of the OS on the board, and so on, can be performed without
     * changing the name of the node. For best practices, on operating systems
     * providing the notion of node name, it is recommended that nodeName be
     * derived from the operating system node name.
     */
    public String nodeName;

    /**
     * Node is, or is not, in the cluster membership.
     */
    public boolean member;

    /**
     * Timestamp at which the node was last booted.
     */
    public long bootTimestamp;

    /**
     *
     * The initial view number, that is, the view number when this
     * configured node joined the cluster and became a member. It is OK for two
     * nodes to have the same initial view numbers. However, it is required that if
     * node N1 is reported to service users as a member before node N2, then
     * initialViewNumber of N1 < initialViewNumber of N2. Some implementations of
     * the Cluster Membership Service may choose not to use the notion of view
     * number. In this case, initialViewNumber must be set to zero.
     */
    public long initialViewNumber;

}
