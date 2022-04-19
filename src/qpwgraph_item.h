// qpwgraph_item.h
//
/****************************************************************************
   Copyright (C) 2021-2022, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#ifndef __qpwgraph_item_h
#define __qpwgraph_item_h

#include <QGraphicsPathItem>

#include <QColor>
#include <QHash>


//----------------------------------------------------------------------------
// qpwgraph_item -- Base graphics item.

class qpwgraph_item : public QGraphicsPathItem
{
public:

	// Constructor.
	qpwgraph_item(QGraphicsItem *parent = nullptr);

	// Basic color accessors.
	void setForeground(const QColor& color);
	const QColor& foreground() const;

	void setBackground(const QColor& color);
	const QColor& background() const;

	// Marking methods.
	void setMarked(bool marked);
	bool isMarked() const;

	// Highlighting methods.
	void setHighlight(bool hilite);
	bool isHighlight() const;

	// Raise item z-value (dynamic always-on-top).
	void raise();

	// Item modes.
	enum Mode { None = 0,
		Input = 1, Output = 2,
		Duplex = Input | Output };

	// Item hash/map key (by id).
	class IdKey
	{
	public:

		// Constructors.
		IdKey (uint id, Mode mode, uint type = 0)
			: m_id(id), m_mode(mode), m_type(type) {}
		IdKey (const IdKey& key)
			: m_id(key.id()), m_mode(key.mode()), m_type(key.type()) {}

		// Key accessors.
		uint id() const
			{ return m_id; }
		Mode mode() const
			{ return m_mode; }
		uint type() const
			{ return m_type; }

		// Hash/map key comparators.
		bool operator== (const IdKey& key) const
		{
			return IdKey::type() == key.type()
				&& IdKey::mode() == key.mode()
				&& IdKey::id()   == key.id();
		}

	private:

		// Key fields.
		uint m_id;
		Mode m_mode;
		uint m_type;
	};

	typedef QHash<IdKey, qpwgraph_item *> IdKeys;

	// Item hash/map key (by name).
	class NameKey
	{
	public:

		// Constructors.
		NameKey (const QString& name, Mode mode, uint type = 0)
			: m_name(name), m_mode(mode), m_type(type) {}
		NameKey (const NameKey& key)
			: m_name(key.name()), m_mode(key.mode()), m_type(key.type()) {}

		// Key accessors.
		const QString& name() const
			{ return m_name; }
		Mode mode() const
			{ return m_mode; }
		uint type() const
			{ return m_type; }

		// Hash/map key comparators.
		bool operator== (const NameKey& key) const
		{
			return NameKey::type() == key.type()
				&& NameKey::mode() == key.mode()
				&& NameKey::name() == key.name();
		}

	private:

		// Key fields.
		QString m_name;
		Mode m_mode;
		uint m_type;
	};

	// Item-type hash (static)
	static uint itemType(const QByteArray& type_name);

	// Rectangular editor extents.
	virtual QRectF editorRect() const;

	// Path and bounding rectangle override.
	void setPath(const QPainterPath& path);

	// Bounding rectangle accessor.
	const QRectF& itemRect() const;

private:

	// Instance variables.
	QColor m_foreground;
	QColor m_background;

	bool m_marked;
	bool m_hilite;

	QRectF m_rect;
};


// Item hash function.
inline uint qHash ( const qpwgraph_item::IdKey& key )
{
	return qHash(key.id()) ^ qHash(uint(key.mode())) ^ qHash(key.type());
}

inline uint qHash ( const qpwgraph_item::NameKey& key )
{
	return qHash(key.name()) ^ qHash(uint(key.mode())) ^ qHash(key.type());
}


#endif	// __qpwgraph_item_h

// end of qpwgraph_item.h
