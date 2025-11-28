/*!
 * \file MessageContext.h
 * \brief typesafe message context used for core messages
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2009 – 2018 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include <string>
#include <memory>

/**
 * \brief Core Message Context
 *
 * Store each type in a derived type
 *
 * This allows typesafety but also allows consumers to use whatever
 * types they need.
 *
 */
struct VPServiceRequestContext
{
};

using VPServiceRequestContextSharedPtr = std::shared_ptr<VPServiceRequestContext>;

/*!
 * \brief Helper context class, not meant to be used directly
 *
 * NOTE: T should be a default constructable and RAII managed type
 * TODO: how to enforce this?
 */
template <typename T>
struct VPServiceRequestContextDerived : public VPServiceRequestContext
{
	/*!
	 *  \brief convenience constructor
	 */
	VPServiceRequestContextDerived(const T &value)
		: m_item (value)
	{
	}

	using ItemType = T;

	// Force item usage through contextItemGet
	template <typename ItemType>
	friend ItemType contextItemGet (const VPServiceRequestContextSharedPtr &context);

private:

	T m_item;
};

/*!
 * \brief Helper method to get context items
 *
 * Instead of allowing direct access to context items, use this
 * wrapper.  It both does the casting as well as returns a default
 * constructed item if the context is empty
 *
 * \return item
 */
template <typename ItemType>
ItemType contextItemGet (const VPServiceRequestContextSharedPtr &context)
{
	// TODO: could do a dynamic cast to help ensure safety
	auto derivedContext = std::static_pointer_cast<VPServiceRequestContextDerived<ItemType>> (context);

	if (derivedContext)
	{
		return derivedContext->m_item;
	}
	else
	{
		return ItemType ();
	}
}
