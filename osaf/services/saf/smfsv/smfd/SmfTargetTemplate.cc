/*
 *
 * (C) Copyright 2009 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include "stdio.h"
#include "logtrace.h"
#include "SmfTargetTemplate.hh"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

//================================================================================
// Class SmfParentType
// Purpose:
// Comments:
//================================================================================

SmfParentType::SmfParentType()
{
}

// ------------------------------------------------------------------------------
// ~SmfParentType()
// ------------------------------------------------------------------------------
SmfParentType::~SmfParentType()
{
}

//------------------------------------------------------------------------------
// setParentDn()
//------------------------------------------------------------------------------
void
SmfParentType::setParentDn(const std::string & i_parentDn)
{
	m_parentDn = i_parentDn;
}

//------------------------------------------------------------------------------
// getParentDn()
//------------------------------------------------------------------------------
const std::string & 
SmfParentType::getParentDn(void) const
{
	return m_parentDn;
}

//------------------------------------------------------------------------------
// setTypeDn()
//------------------------------------------------------------------------------
void 
SmfParentType::setTypeDn(const std::string & i_typeDn)
{
	m_typeDn = i_typeDn;
}

//------------------------------------------------------------------------------
// getTypeDn()
//------------------------------------------------------------------------------
const std::string & 
SmfParentType::getTypeDn(void) const 
{
	return m_typeDn;
}

//================================================================================
// Class SmfBundleRef
// Purpose:
// Comments:
//================================================================================
SmfBundleRef::SmfBundleRef()
{
}

// ------------------------------------------------------------------------------
// ~SmfBundleRef()
// ------------------------------------------------------------------------------
SmfBundleRef::~SmfBundleRef()
{
}

//------------------------------------------------------------------------------
// setBundleDn()
//------------------------------------------------------------------------------
void
SmfBundleRef::setBundleDn(const std::string & i_bundleDn)
{
	m_bundleDn = i_bundleDn;
}

//------------------------------------------------------------------------------
// getBundleDn()
//------------------------------------------------------------------------------
const std::string & 
SmfBundleRef::getBundleDn(void) const 
{
	return m_bundleDn;
}

//------------------------------------------------------------------------------
// setPathNamePrefix()
//------------------------------------------------------------------------------
void 
SmfBundleRef::setPathNamePrefix(const std::string & i_prefix)
{
	m_pathNamePrefix = i_prefix;
}

//------------------------------------------------------------------------------
// getPathNamePrefix()
//------------------------------------------------------------------------------
const std::string & 
SmfBundleRef::getPathNamePrefix(void) const 
{
	return m_pathNamePrefix;
}

//================================================================================
// Class SmfTargetNodeTemplate
//================================================================================
SmfTargetNodeTemplate::SmfTargetNodeTemplate()
{
}

// ------------------------------------------------------------------------------
// ~SmfTargetNodeTemplate()
// ------------------------------------------------------------------------------
SmfTargetNodeTemplate::~SmfTargetNodeTemplate()
{
	std::list < SmfBundleRef * >::iterator it;

	/* Delete remove bundle references */
	for (it = m_swRemove.begin(); it != m_swRemove.end(); ++it) {
		delete(*it);
	}

	/* Delete install bundle references */
	for (it = m_swInstall.begin(); it != m_swInstall.end(); ++it) {
		delete(*it);
	}

	std::list < SmfParentType * >::iterator auit;

	/* Delete AU templates */
	for (auit = m_activationUnitTemplate.begin(); auit != m_activationUnitTemplate.end(); ++auit) {
		delete(*auit);
	}
}

//------------------------------------------------------------------------------
// setObjectDn()
//------------------------------------------------------------------------------
void
SmfTargetNodeTemplate::setObjectDn(const std::string & i_objectDn)
{
	m_objectDn = i_objectDn;
}

//------------------------------------------------------------------------------
// getObjectDn()
//------------------------------------------------------------------------------
const std::string & 
SmfTargetNodeTemplate::getObjectDn(void) const 
{
	return m_objectDn;
}

//------------------------------------------------------------------------------
// addActivationUnitTemplate()
//------------------------------------------------------------------------------
void 
SmfTargetNodeTemplate::addActivationUnitTemplate(SmfParentType * i_activationUnit)
{
	m_activationUnitTemplate.push_back(i_activationUnit);
}

//------------------------------------------------------------------------------
// getPathNamePrefix()
//------------------------------------------------------------------------------
const std::list < SmfParentType * >&
SmfTargetNodeTemplate::getActivationUnitTemplateList(void) const
{
	return m_activationUnitTemplate;
}

//------------------------------------------------------------------------------
// addSwRemove()
//------------------------------------------------------------------------------
void 
SmfTargetNodeTemplate::addSwRemove(SmfBundleRef * i_bundle)
{
	m_swRemove.push_back(i_bundle);
}

//------------------------------------------------------------------------------
// getSwRemoveList()
//------------------------------------------------------------------------------
const std::list < SmfBundleRef * >&
SmfTargetNodeTemplate::getSwRemoveList(void) const
{
	return m_swRemove;
}

//------------------------------------------------------------------------------
// addSwInstall()
//------------------------------------------------------------------------------
void 
SmfTargetNodeTemplate::addSwInstall(SmfBundleRef * i_bundle)
{
	m_swInstall.push_back(i_bundle);
}

//------------------------------------------------------------------------------
// getSwInstallList()
//------------------------------------------------------------------------------
const std::list < SmfBundleRef * >&
SmfTargetNodeTemplate::getSwInstallList(void) const
{
	return m_swInstall;
}

//================================================================================
// Class SmfTargetEntityTemplate
//================================================================================
SmfTargetEntityTemplate::SmfTargetEntityTemplate():
	m_entityTemplate(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfTargetEntityTemplate()
// ------------------------------------------------------------------------------
SmfTargetEntityTemplate::~SmfTargetEntityTemplate()
{
	delete m_entityTemplate;

	std::list < SmfImmModifyOperation * >::iterator it;

	/* Delete IMM operations */
	for (it = m_modifyOperations.begin(); it != m_modifyOperations.end(); ++it) {
		delete(*it);
	}
}

//------------------------------------------------------------------------------
// setEntityTemplate()
//------------------------------------------------------------------------------
void
SmfTargetEntityTemplate::setEntityTemplate(SmfParentType * i_entity)
{
	m_entityTemplate = i_entity;
}

//------------------------------------------------------------------------------
// getEntityTemplate()
//------------------------------------------------------------------------------
const SmfParentType *
SmfTargetEntityTemplate::getEntityTemplate(void) const 
{
	return m_entityTemplate;
}

//------------------------------------------------------------------------------
// addModifyOperation()
//------------------------------------------------------------------------------
void 
SmfTargetEntityTemplate::addModifyOperation(SmfImmModifyOperation * i_op)
{
	m_modifyOperations.push_back(i_op);
}

//------------------------------------------------------------------------------
// getModifyOperationList()
//------------------------------------------------------------------------------
const std::list < SmfImmModifyOperation * >&
SmfTargetEntityTemplate::getModifyOperationList(void) const
{
	return m_modifyOperations;
}
