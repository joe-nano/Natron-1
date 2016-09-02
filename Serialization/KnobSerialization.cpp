/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */


#include <cfloat>
#include <climits>
#include <iostream>

#include "KnobSerialization.h"

#include "Serialization/CurveSerialization.h"

SERIALIZATION_NAMESPACE_ENTER;

const std::string&
ValueSerialization::getKnobName() const
{
    return _serialization->getName();
}


void
KnobSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    if (!_mustSerialize) {
        return;
    }
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << _scriptName;

    // Instead of inserting in the map each boolean
    // we serialize strings, meaning that
    // if they are here, their value is true otherwise it goes to default
    std::list<std::string> propNames;
    if (_visibilityChanged && _isPersistent) {
        propNames.push_back("VisibilityChanged");
    }
    if (_masterIsAlias) {
        propNames.push_back("MasterIsAlias");
    }
    assert((int)_values.size() == _dimension);

    bool hasDimensionToSerialize = false;
    for (std::size_t i = 0; i < _values.size(); ++i) {
        if (_values[i]._mustSerialize && _isPersistent) {
            hasDimensionToSerialize = true;
            break;
        }
    }
    if (hasDimensionToSerialize) {
        // We serialize a value, we need to know which type of knob this is because at the time of deserialization the
        // application may not have created the corresponding knob already and may not know to what type it corresponds
        em << YAML_NAMESPACE::Key << "Dimensions" << YAML_NAMESPACE::Value;
        // Starting dimensions
        em << YAML_NAMESPACE::BeginSeq;
        for (std::size_t i = 0; i < _values.size(); ++i) {
            const ValueSerialization& s = _values[i];

            if (!s._mustSerialize) {
                continue;
            }

            em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginMap;

            std::list<std::string> dimensionProps;
            em << YAML_NAMESPACE::Key << "Index" << YAML_NAMESPACE::Value << s._dimension;

            if (s._serializeValue && s._animationCurve.keys.empty() && s._type != ValueSerialization::eSerializationValueVariantTypeNone) {
                em << YAML_NAMESPACE::Key << "Value" << YAML_NAMESPACE::Value;
                switch (s._type) {
                    case ValueSerialization::eSerializationValueVariantTypeBoolean:
                        em << s._value.isBool;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeInteger:
                        em << s._value.isInt;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeDouble:
                        em << s._value.isDouble;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeString:
                        em << s._value.isString;
                        break;
                    default:
                        break;
                }
            }
            if (s._serializeDefaultValue && s._type != ValueSerialization::eSerializationValueVariantTypeNone) {
                em << YAML_NAMESPACE::Key << "Default" << YAML_NAMESPACE::Value;
                switch (s._type) {
                    case ValueSerialization::eSerializationValueVariantTypeBoolean:
                        em << s._defaultValue.isBool;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeInteger:
                        em << s._defaultValue.isInt;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeDouble:
                        em << s._defaultValue.isDouble;
                        break;
                    case ValueSerialization::eSerializationValueVariantTypeString:
                        em << s._defaultValue.isString;
                        break;
                    default:
                        break;
                }
            }
            if (!s._animationCurve.keys.empty()) {
                // Serialize only if non empty
                em << YAML_NAMESPACE::Key << "Curve" << YAML_NAMESPACE::Value;
                s._animationCurve.encode(em);
            }
            if (!s._expression.empty()) {
                // Serialize only if non empty
                em << YAML_NAMESPACE::Key << "Expr" << YAML_NAMESPACE::Value << s._expression;
                if (s._expresionHasReturnVariable) {
                    // Serialize only if true because otherwise we assume false by default
                    dimensionProps.push_back("ExprHasRet");
                }
            }
            if (s._enabledChanged) {
                dimensionProps.push_back("EnabledChanged");
            }
            if (!s._slaveMasterLink.masterKnobName.empty()) {
                em << YAML_NAMESPACE::Key << "Master" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
                em << s._slaveMasterLink.masterKnobName;
                em << s._slaveMasterLink.masterDimension;
                em << s._slaveMasterLink.masterNodeName;
                if (!s._slaveMasterLink.masterTrackName.empty()) {
                    em << s._slaveMasterLink.masterTrackName;
                }
                em << YAML_NAMESPACE::EndSeq;
            }
            if (!dimensionProps.empty()) {
                em << YAML_NAMESPACE::Key << "DimProps" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
                for (std::list<std::string>::const_iterator it2 = dimensionProps.begin(); it2 != dimensionProps.end(); ++it2) {
                    em << *it2;
                }
                em << YAML_NAMESPACE::EndSeq;
            }

            em << YAML_NAMESPACE::EndMap;
        } // for each dimension
        em << YAML_NAMESPACE::EndSeq;
    } // hasDimensionToSerialize

    TextExtraData* textData = dynamic_cast<TextExtraData*>(_extraData.get());
    if (_extraData && _isPersistent) {
        ParametricExtraData* parametricData = dynamic_cast<ParametricExtraData*>(_extraData.get());
        if (parametricData) {
            if (!parametricData->parametricCurves.empty()) {
                em << YAML_NAMESPACE::Key << "ParametricCurves" << YAML_NAMESPACE::Value;
                em << YAML_NAMESPACE::BeginSeq;
                for (std::list<CurveSerialization>::const_iterator it = parametricData->parametricCurves.begin(); it!=parametricData->parametricCurves.end(); ++it) {
                    it->encode(em);
                }
                em << YAML_NAMESPACE::EndSeq;
            }
        } else if (textData) {
            if (!textData->keyframes.empty()) {
                em << YAML_NAMESPACE::Key << "TextAnim" << YAML_NAMESPACE::Value;
                em << YAML_NAMESPACE::Flow;
                em << YAML_NAMESPACE::BeginSeq;
                for (std::map<int, std::string>::const_iterator it = textData->keyframes.begin(); it!=textData->keyframes.end(); ++it) {
                    em << it->first << it->second;
                }
                em << YAML_NAMESPACE::EndSeq;
            }
            if (std::abs(textData->fontColor[0] - 0.) > 0.01 || std::abs(textData->fontColor[1] - 0.) > 0.01 || std::abs(textData->fontColor[2] - 0.) > 0.01) {
                em << YAML_NAMESPACE::Key << "FontColor" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << textData->fontColor[0] << textData->fontColor[1] << textData->fontColor[2] << YAML_NAMESPACE::EndSeq;
            }
            if (textData->fontSize != kKnobStringDefaultFontSize) {
                em << YAML_NAMESPACE::Key << "FontSize" << YAML_NAMESPACE::Value << textData->fontSize;
            }
            if (textData->fontFamily != NATRON_FONT) {
                em << YAML_NAMESPACE::Key << "Font" << YAML_NAMESPACE::Value << textData->fontFamily;
            }
            if (textData->italicActivated) {
                propNames.push_back("Italic");
            }
            if (textData->boldActivated) {
                propNames.push_back("Bold");
            }
        }
    }

    if (_isUserKnob) {
        // Num of dimensions can be figured out for knobs created by plug-in
        // Knobs created by user need to know it
        em << YAML_NAMESPACE::Key << "NDims" << YAML_NAMESPACE::Value << _dimension;
        em << YAML_NAMESPACE::Key << "TypeName" << YAML_NAMESPACE::Value << _typeName;

        if (_label != _scriptName) {
            em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << _label;
        }
        if (!_tooltip.empty()) {
            em << YAML_NAMESPACE::Key << "Hint" << YAML_NAMESPACE::Value << _tooltip;
        }
        if (!_triggerNewLine) {
            propNames.push_back("NoNewLine");
        }
        if (!_evaluatesOnChange) {
            propNames.push_back("NoEval");
        }
        if (_animatesChanged) {
            propNames.push_back("AnimatesChanged");
        }
        if (!_isPersistent) {
            propNames.push_back("Volatile");
        }
        if (!_iconFilePath[0].empty()) {
            em << YAML_NAMESPACE::Key << "UncheckedIcon" << YAML_NAMESPACE::Value << _iconFilePath[0];
        }
        if (!_iconFilePath[1].empty()) {
            em << YAML_NAMESPACE::Key << "CheckedIcon" << YAML_NAMESPACE::Value << _iconFilePath[1];
        }

        TypeExtraData* typeData = _extraData.get();
        ChoiceExtraData* cdata = dynamic_cast<ChoiceExtraData*>(_extraData.get());
        ValueExtraData* vdata = dynamic_cast<ValueExtraData*>(_extraData.get());
        TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData.get());
        FileExtraData* fdata = dynamic_cast<FileExtraData*>(_extraData.get());
        PathExtraData* pdata = dynamic_cast<PathExtraData*>(_extraData.get());

        if (cdata) {
            if (!cdata->_entries.empty()) {
                em << YAML_NAMESPACE::Key << "Entries" << YAML_NAMESPACE::Value;
                em << YAML_NAMESPACE::BeginSeq;
                for (std::size_t i = 0; i < cdata->_entries.size(); ++i) {
                    em << cdata->_entries[i];
                }
                em << YAML_NAMESPACE::EndSeq;
            }
            if (!cdata->_helpStrings.empty()) {
                em << YAML_NAMESPACE::Key << "Hints" << YAML_NAMESPACE::Value;
                em << YAML_NAMESPACE::BeginSeq;
                for (std::size_t i = 0; i < cdata->_helpStrings.size(); ++i) {
                    em << cdata->_helpStrings[i];
                }
                em << YAML_NAMESPACE::EndSeq;
            }
        } else if (vdata) {
            if (vdata->min != INT_MIN && vdata->min != -DBL_MAX) {
                em << YAML_NAMESPACE::Key << "Min" << YAML_NAMESPACE::Value << vdata->min;
            }
            if (vdata->min != INT_MAX && vdata->min != DBL_MAX) {
                em << YAML_NAMESPACE::Key << "Max" << YAML_NAMESPACE::Value << vdata->max;
            }
            if (vdata->min != INT_MIN && vdata->min != -DBL_MAX) {
                em << YAML_NAMESPACE::Key << "DisplayMin" << YAML_NAMESPACE::Value << vdata->dmin;
            }
            if (vdata->min != INT_MAX && vdata->min != DBL_MAX) {
                em << YAML_NAMESPACE::Key << "DisplayMax" << YAML_NAMESPACE::Value << vdata->dmax;
            }
        } else if (tdata) {
            if (tdata->label) {
                propNames.push_back("IsLabel");
            }
            if (tdata->multiLine) {
                propNames.push_back("MultiLine");
            }
            if (tdata->richText) {
                propNames.push_back("RichText");
            }
        } else if (pdata) {
            if (pdata->multiPath) {
                propNames.push_back("MultiPath");
            }
        } else if (fdata) {
            if (fdata->useSequences) {
                propNames.push_back("Sequences");
            }
        }

        if (typeData) {
            if (typeData->useHostOverlayHandle) {
                propNames.push_back("UseOverlay");
            }
        }

    } // if (_isUserKnob) {

    if (_hasViewerInterface) {
        if (!_inViewerContextItemLayout.empty() && _inViewerContextItemLayout != kInViewerContextItemLayoutSpacing) {
            em << YAML_NAMESPACE::Key << "InViewerLayout" << YAML_NAMESPACE::Value << _inViewerContextItemLayout;
        } else {
            if (_inViewerContextItemSpacing != kKnobInViewerContextDefaultItemSpacing) {
                em << YAML_NAMESPACE::Key << "InViewerSpacing" << YAML_NAMESPACE::Value << _inViewerContextItemSpacing;
            }
        }
        if (_isUserKnob) {
            if (!_inViewerContextLabel.empty()) {
                em << YAML_NAMESPACE::Key << "InViewerLabel" << YAML_NAMESPACE::Value << _inViewerContextLabel;
            }
            if (!_inViewerContextIconFilePath[0].empty()) {
                em << YAML_NAMESPACE::Key << "InViewerIconUnchecked" << YAML_NAMESPACE::Value << _inViewerContextIconFilePath[0];
            }
            if (!_inViewerContextIconFilePath[1].empty()) {
                em << YAML_NAMESPACE::Key << "InViewerIconChecked" << YAML_NAMESPACE::Value << _inViewerContextIconFilePath[1];
            }
        }
    }

    if (!propNames.empty()) {
        em << YAML_NAMESPACE::Key << "Props" << YAML_NAMESPACE::Value << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it2 = propNames.begin(); it2 != propNames.end(); ++it2) {
            em << *it2;
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    em << YAML_NAMESPACE::EndMap;
} // KnobSerialization::encode

template <typename T>
static T* getOrCreateExtraData(boost::scoped_ptr<TypeExtraData>& extraData)
{
    T* data = 0;
    if (extraData.get()) {
        data = dynamic_cast<T*>(extraData.get());
    } else {
        data = new T;
        extraData.reset(data);
    }
    assert(data);
    if (!data) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    return data;
}

static void decodeValueFromNode(const YAML_NAMESPACE::Node& node,
                                SerializationValueVariant& variant, ValueSerialization::SerializationValueVariantTypeEnum* type)
{
    // yaml-cpp looses the original type information and does not seem to make a difference whether
    // the value was a string or a POD scalar. All functions as<T>() will succeed.
    // See https://github.com/jbeder/yaml-cpp/issues/261

    try {
        *type = ValueSerialization::eSerializationValueVariantTypeDouble;
        variant.isDouble = node.as<double>();
        variant.isInt = (int)variant.isDouble; //node.as<int>();
    } catch (const YAML_NAMESPACE::BadConversion&) {
        try {
            variant.isBool = node.as<bool>();
            *type = ValueSerialization::eSerializationValueVariantTypeBoolean;
        } catch (const YAML_NAMESPACE::BadConversion&) {
            variant.isString = node.as<std::string>();
            *type = ValueSerialization::eSerializationValueVariantTypeString;

        }
    }
    
}

void
KnobSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsMap()) {
        return;
    }

    // Set the flag to true if the user use this object directly to encode after
    _mustSerialize = true;

    _scriptName = node["ScriptName"].as<std::string>();


    if (node["Dimensions"]) {

        YAML_NAMESPACE::Node dimensionsNode = node["Dimensions"];
        if (!dimensionsNode.IsSequence()) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        _values.resize(dimensionsNode.size());
        for (std::size_t i = 0; i < _values.size(); ++i) {
            _values[i]._serialization = this;
            _values[i]._mustSerialize = true;
            _values[i]._dimension = i;
        }
        for (std::size_t i = 0; i < dimensionsNode.size(); ++i) {

            YAML_NAMESPACE::Node dimNode = dimensionsNode[i];

            if (!dimNode.IsMap()) {
                throw YAML_NAMESPACE::InvalidNode();
            }
            int dim = dimNode["Index"].as<int>();
            ValueSerialization& s = _values[i];
            s._dimension = dim;
            if (dimNode["Value"]) {
                YAML_NAMESPACE::Node valueNode = dimNode["Value"];
                decodeValueFromNode(valueNode, s._value, &s._type);
                s._serializeValue = true;
            }
            if (dimNode["Default"]) {
                YAML_NAMESPACE::Node valueNode = dimNode["Default"];
                decodeValueFromNode(valueNode, s._defaultValue, &s._type);
                s._serializeDefaultValue = true;
            }
            if (dimNode["Curve"]) {
                YAML_NAMESPACE::Node curveNode = dimNode["Curve"];
                s._animationCurve.decode(curveNode);
            }
            if (dimNode["Expr"]) {
                s._expression = dimNode["Expr"].as<std::string>();
        
            }
            if (dimNode["Master"]) {
                YAML_NAMESPACE::Node masterNode = dimNode["Master"];
                if (!masterNode.IsSequence() || (masterNode.size() != 3 && masterNode.size() != 4)) {
                    throw YAML_NAMESPACE::InvalidNode();
                }

                s._slaveMasterLink.masterKnobName = masterNode[0].as<std::string>();
                s._slaveMasterLink.masterDimension = masterNode[1].as<int>();
                s._slaveMasterLink.masterNodeName = masterNode[2].as<std::string>();
                if (masterNode.size() == 4) {
                    s._slaveMasterLink.masterTrackName = masterNode[3].as<std::string>();
                }


            }

            if (dimNode["DimProps"]) {
                YAML_NAMESPACE::Node propsNode = dimNode["DimProps"];
                for (std::size_t j = 0; j < propsNode.size(); ++j) {
                    std::string prop = propsNode[j].as<std::string>();
                    if (prop == "EnabledChanged") {
                        s._enabledChanged = true;
                    } else if (prop == "ExprHasRet") {
                        s._expresionHasReturnVariable = true;
                    } else {
                        assert(false);
                        std::cerr << "WARNING: Unknown parameter property " << prop << std::endl;
                    }
                }
            }

        } // for all serialized dimensions
    } // has dimension serialized

    if (node["ParametricCurves"]) {
        YAML_NAMESPACE::Node curveNode = node["ParametricCurves"];
        ParametricExtraData *data = getOrCreateExtraData<ParametricExtraData>(_extraData);
        for (std::size_t i = 0; i < curveNode.size(); ++i) {
            CurveSerialization s;
            s.decode(curveNode[i]);
            data->parametricCurves.push_back(s);
        }
    }
    if (node["TextAnim"]) {
        YAML_NAMESPACE::Node curveNode = node["TextAnim"];
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        // If type = 0 we expect a int, otherwise a string
        int type = 0;
        std::pair<int, std::string> p;
        for (std::size_t i = 0; i < curveNode.size(); ++i) {
            if (type == 0) {
                p.first = curveNode[i].as<int>();
                type = 1;
            } else if (type == 1) {
                type = 0;
                p.second = curveNode[i].as<std::string>();
                data->keyframes.insert(p);
            }
        }
    }

    if (node["FontColor"]) {
        YAML_NAMESPACE::Node n = node["FontColor"];
        if (n.size() != 3) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        data->fontColor[0] = n[0].as<double>();
        data->fontColor[1] = n[1].as<double>();
        data->fontColor[2] = n[2].as<double>();

    }
    if (node["FontSize"]) {
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        data->fontSize = node["FontSize"].as<int>();
    }

    if (node["Font"]) {
        TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
        data->fontFamily = node["Font"].as<std::string>();
    }

    if (node["NDims"]) {

        // This is a user knob
        _isUserKnob = true;

        _typeName = node["TypeName"].as<std::string>();

        _dimension = node["NDims"].as<int>();

        if (node["Label"]) {
            _label = node["Label"].as<std::string>();
        } else {
            _label = _scriptName;
        }
        if (node["Hint"]) {
            _tooltip = node["Hint"].as<std::string>();
        }

        if (node["Persistent"]) {
            _isPersistent = node["Persistent"].as<bool>();
        } else {
            _isPersistent = true;
        }
        if (node["UncheckedIcon"]) {
            _iconFilePath[0] = node["UncheckedIcon"].as<std::string>();
        }
        if (node["CheckedIcon"]) {
            _iconFilePath[1] = node["CheckedIcon"].as<std::string>();
        }

        // User specific data
        if (node["Entries"]) {
            // This is a choice
            ChoiceExtraData *data = new ChoiceExtraData;
            _extraData.reset(data);
            YAML_NAMESPACE::Node entriesNode = node["Entries"];
            for (std::size_t i = 0; i < entriesNode.size(); ++i) {
                data->_entries.push_back(entriesNode[i].as<std::string>());
            }

            // Also look for hints...
            if (node["Hints"]) {
                YAML_NAMESPACE::Node hintsNode = node["Hints"];
                for (std::size_t i = 0; i < hintsNode.size(); ++i) {
                    data->_helpStrings.push_back(hintsNode[i].as<std::string>());
                }
            }
        }

        if (node["Min"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->min = node["Min"].as<double>();
        }
        if (node["Max"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->max = node["Max"].as<double>();
        }
        if (node["DisplayMin"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->dmin = node["DisplayMin"].as<double>();
        }
        if (node["DisplayMax"]) {
            ValueExtraData* data = getOrCreateExtraData<ValueExtraData>(_extraData);
            data->dmax = node["DisplayMax"].as<double>();
        }
    } // isUserKnob

    if (node["InViewerLayout"]) {
        _inViewerContextItemLayout = node["InViewerLayout"].as<std::string>();
        _hasViewerInterface = true;
    }

    if (node["InViewerSpacing"]) {
        _inViewerContextItemSpacing = node["InViewerSpacing"].as<int>();
        _hasViewerInterface = true;
    }

    if (_isUserKnob) {
        if (node["InViewerLabel"]) {
            _inViewerContextLabel = node["InViewerLabel"].as<std::string>();
            _hasViewerInterface = true;
        }
        if (node["InViewerIconUnchecked"]) {
            _inViewerContextIconFilePath[0] = node["InViewerIconUnchecked"].as<std::string>();
            _hasViewerInterface = true;
        }
        if (node["InViewerIconChecked"]) {
            _inViewerContextIconFilePath[1] = node["InViewerIconChecked"].as<std::string>();
            _hasViewerInterface = true;
        }
    }

    if (node["Props"]) {
        YAML_NAMESPACE::Node propsNode = node["Props"];
        for (std::size_t i = 0; i < propsNode.size(); ++i) {
            std::string prop = propsNode[i].as<std::string>();
            if (prop == "VisibilityChanged") {
                _visibilityChanged = true;
            } else if (prop == "MasterIsAlias") {
                _masterIsAlias = true;
            } else if (prop == "NoNewLine") {
                _triggerNewLine = false;
            } else if (prop == "NoEval") {
                _evaluatesOnChange = false;
            } else if (prop == "AnimatesChanged") {
                _animatesChanged = true;
            } else if (prop == "Volatile") {
                _isPersistent = false;
            } else if (prop == "IsLabel") {
                TextExtraData* data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->label = true;
                data->multiLine = false;
                data->richText = false;
            } else if (prop == "MultiLine") {
                TextExtraData* data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->label = false;
                data->multiLine = true;
                data->richText = false;
            } else if (prop == "RichText") {
                TextExtraData* data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->richText = true;
            } else if (prop == "MultiPath") {
                PathExtraData* data = getOrCreateExtraData<PathExtraData>(_extraData);
                data->multiPath = node["MultiPath"].as<bool>();
            } else if (prop == "Sequences") {
                FileExtraData* data = getOrCreateExtraData<FileExtraData>(_extraData);
                data->useSequences = node["Sequences"].as<bool>();
            } else if (prop == "UseOverlay") {
                TypeExtraData* data = getOrCreateExtraData<TypeExtraData>(_extraData);
                data->useHostOverlayHandle = true;
            } else if (prop == "Italic") {
                TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->italicActivated = true;
            } else if (prop == "Bold") {
                TextExtraData *data = getOrCreateExtraData<TextExtraData>(_extraData);
                data->boldActivated = true;
            } else {
                assert(false);
                std::cerr << "WARNING: Unrecognized parameter property " << prop << std::endl;
            }


        }

    }
    
} // KnobSerialization::decode


void
GroupKnobSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::VerbatimTag("GroupParam");
    em << YAML_NAMESPACE::BeginMap;

    em << YAML_NAMESPACE::Key << "TypeName" << YAML_NAMESPACE::Value << _typeName;
    em << YAML_NAMESPACE::Key << "ScriptName" << YAML_NAMESPACE::Value << _name;
    if (_label != _name) {
        em << YAML_NAMESPACE::Key << "Label" << YAML_NAMESPACE::Value << _label;
    }

    if (!_children.empty()) {
        em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = _children.begin(); it!=_children.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    
    
    std::list<std::string> props;
    if (_isOpened) {
        props.push_back("Opened");
    }
    if (_isSetAsTab) {
        props.push_back("IsTab");
    }
    if (_secret) {
        props.push_back("Secret");
    }
    if (!props.empty()) {
        em << YAML_NAMESPACE::Key << "Props" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (std::list<std::string>::const_iterator it = props.begin(); it!=props.end(); ++it) {
            em << *it;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;
} // GroupKnobSerialization::encode

void
GroupKnobSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    _typeName = node["TypeName"].as<std::string>();
    _name = node["ScriptName"].as<std::string>();
    if (node["Label"]) {
        _label = node["Label"].as<std::string>();
    } else {
        _label = _name;
    }

    if (node["Params"]) {
        YAML_NAMESPACE::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            if (paramsNode[i].Tag() == "GroupParam") {
                GroupKnobSerializationPtr s(new GroupKnobSerialization);
                s->decode(paramsNode[i]);
                _children.push_back(s);
            } else {
                KnobSerializationPtr s (new KnobSerialization);
                s->decode(paramsNode[i]);
                _children.push_back(s);
            }
        }
    }

    if (node["Props"]) {
        YAML_NAMESPACE::Node propsNode = node["Props"];
        for (std::size_t i = 0; i < propsNode.size(); ++i) {
            std::string prop = propsNode[i].as<std::string>();
            if (prop == "Opened") {
                _isOpened = true;
            } else if (prop == "Secret") {
                _secret = true;
            } else if (prop == "IsTab") {
                _isSetAsTab = true;
            }
        }
    }
} // GroupKnobSerialization::decode

SERIALIZATION_NAMESPACE_EXIT;
