/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey, Eclipse and Lycium engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

/** @file graphics/aurora/modelnode.h
 *  A node within a 3D model.
 */

#ifndef GRAPHICS_AURORA_MODELNODE_H
#define GRAPHICS_AURORA_MODELNODE_H

#include <list>
#include <vector>

#include "common/ustring.h"
#include "common/boundingbox.h"

#include "graphics/types.h"
#include "graphics/indexbuffer.h"
#include "graphics/vertexbuffer.h"

#include "graphics/aurora/types.h"
#include "graphics/aurora/textureman.h"

namespace Graphics {

namespace Aurora {

class Model;

struct PositionKeyFrame {
	float time;
	glm::vec3 position;
};

struct QuaternionKeyFrame {
	float time;
	glm::vec4 quaternion;
};

class ModelNode {
public:
	ModelNode(Model &model);
	virtual ~ModelNode();

	/** Get the node's name. */
	const Common::UString &getName() const;

	glm::vec3 getSize() const; ///< Get the [width,height,depth] of the node's bounding box.

	/** Should the node never be rendered at all? */
	void setInvisible(bool invisible);

	/** Add another model as a child to this node. */
	void addChild(Model *model);

	// Positioning

	/** Get the position of the node. */
	glm::vec3 getPosition() const;
	/** Get the rotation of the node. */
	glm::vec3 getRotation() const;
	/** Get the orientation of the node. */
	glm::vec4 getOrientation() const;

	/** Get the position of the node after translate/rotate. */
	glm::vec3 getAbsolutePosition() const;

	/** Set the position of the node. */
	void setPosition(const glm::vec3 &position);
	/** Set the rotation of the node. */
	void setRotation(const glm::vec3 &rotation);
	/** Set the orientation of the node. */
	void setOrientation(const glm::vec3 &axis, float angle);

	/** Move the node, relative to its current position. */
	void move  (const glm::vec3 &amount);
	/** Rotate the node, relative to its current rotation. */
	void rotate(const glm::vec3 &amount);


protected:
	Model *_model; ///< The model this node belongs to.

	ModelNode *_parent;               ///< The node's parent.
	std::list<ModelNode *> _children; ///< The node's children.

	uint32 _level;

	Common::UString _name; ///< The node's name.

	VertexBuffer _vertexBuffer; ///< Node geometry vertex buffer.
	IndexBuffer _indexBuffer;   ///< Node geometry index buffer.

	glm::vec3 _center;      ///< The node's center.
	glm::vec3 _position;    ///< Position of the node.
	glm::vec3 _rotation;    ///< Node rotation.
	glm::vec4 _orientation; ///< Orientation of the node.

	std::vector<PositionKeyFrame> _positionFrames; ///< Keyframes for position animation
	std::vector<QuaternionKeyFrame> _orientationFrames; ///< Keyframes for orientation animation

	/** Position of the node after translate/rotate. */
	glm::mat4 _absolutePosition;

	float _wirecolor[3]; ///< Color of the wireframe.
	float _ambient  [3]; ///< Ambient color.
	float _diffuse  [3]; ///< Diffuse color.
	float _specular [3]; ///< Specular color.
	float _selfIllum[3]; ///< Self illumination color.
	float _shininess;    ///< Shiny?

	std::vector<TextureHandle> _textures; ///< Textures.

	bool _isTransparent;

	bool _dangly; ///< Is the node mesh's dangly?

	float _period;
	float _tightness;
	float _displacement;

	bool _showdispl;
	int  _displtype;

	std::vector<float> _constraints;

	int _tilefade;

	float _scale;

	bool _render; ///< Render the node?
	bool _shadow; ///< Does the node have a shadow?

	bool _beaming;
	bool _inheritcolor;
	bool _rotatetexture;

	float _alpha;

	bool _hasTransparencyHint;
	bool _transparencyHint;

	Common::BoundingBox _boundBox;
	Common::BoundingBox _absoluteBoundBox;


	// Loading helpers
	void loadTextures(const std::vector<Common::UString> &textures);
	void createBound();
	void createCenter();

	void render(RenderPass pass);


private:
	const Common::BoundingBox &getAbsoluteBound() const;
	void createAbsoluteBound(Common::BoundingBox parentPosition);

	void orderChildren();

	void renderGeometry();


public:
	// General helpers

	ModelNode *getParent();             ///< Get the node's parent.
	const ModelNode *getParent() const; ///< Get the node's parent.

	void setParent(ModelNode *parent); ///< Set the node's parent.

	/** Is this node in front of that other node? */
	bool isInFrontOf(const ModelNode &node) const;

	void inheritPosition(ModelNode &node) const;
	void inheritOrientation(ModelNode &node) const;
	void inheritGeometry(ModelNode &node) const;

	void reparent(ModelNode &parent);

	// Animation helpers
	glm::vec3 interpolatePosition(float time) const;
	glm::vec4 interpolateOrientation(float time) const;

	friend class Model;
};

} // End of namespace Aurora

} // End of namespace Graphics

#endif // GRAPHICS_AURORA_MODELNODE_H
