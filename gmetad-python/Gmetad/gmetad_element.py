#/*******************************************************************************
#* Portions Copyright (C) 2008 Novell, Inc. All rights reserved.
#*
#* Redistribution and use in source and binary forms, with or without
#* modification, are permitted provided that the following conditions are met:
#*
#*  - Redistributions of source code must retain the above copyright notice,
#*    this list of conditions and the following disclaimer.
#*
#*  - Redistributions in binary form must reproduce the above copyright notice,
#*    this list of conditions and the following disclaimer in the documentation
#*    and/or other materials provided with the distribution.
#*
#*  - Neither the name of Novell, Inc. nor the names of its
#*    contributors may be used to endorse or promote products derived from this
#*    software without specific prior written permission.
#*
#* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
#* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#* ARE DISCLAIMED. IN NO EVENT SHALL Novell, Inc. OR THE CONTRIBUTORS
#* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#* POSSIBILITY OF SUCH DAMAGE.
#*
#* Authors: Matt Ryan (mrayn novell.com)
#*                  Brad Nicholes (bnicholes novell.com)
#******************************************************************************/

import copy

class Element:
    ''' This class implements the node element that is used to create the data store tree structure.'''
    
    def generateKey(vals):
        ''' This methods generates a node key based on the node id and name'''
        if isinstance(vals,list):
            return ':'.join(vals)
        return vals
    generateKey = staticmethod(generateKey)
    
    def __init__(self, id, attrs, tag=None):
        ''' This is the initialization method '''
        # Initialize the id and tag for the node
        self.id = id
        if tag is None:
            self.tag = id
        else:
            self.tag = tag
        # If any attributes where given during intialization, add them here.
        self.attrs = {}
        self.lastReportedTime = 0
        for k,v in attrs.items():
            self.attrs[k.lower()] = v
        self.children = {}
        self.gridDepth = -1
        
    def __setitem__(self, k, v):
        ''' This method adds or updates an attribute for the node. '''
        try:
            self.children[k].update(v)
        except KeyError:
            self.children[k] = v
            
    def __getitem__(self, k):
        ''' This method retrieves a specific child node. '''
        return self.children[k]
        
    def update(self, elem):
        ''' This method updates an existing chld node based on a new node. '''
        for k in self.attrs.keys():
            try:
                self.attrs[k] = elem.attrs[k]
            except ValueError:
                pass
        
    def __str__(self):
        ''' This method generates a string representation of a node. '''
        if self.attrs.has_key('name'):
            return Element.generateKey([self.id,self.attrs['name']])
        return Element.generateKey(self.id)
        
    def __iter__(self):
        ''' This method allow the class to be an interator over it's children. '''
        return self.children.itervalues()
        
    def __copy__(self):
        ''' Shallow copy method, may not be used. '''
        cp = Element(self.id, {})
        for k in self.attrs.keys():
            try:
                cp.attrs[k.lower()] = copy.copy(self.attrs[k])
            except ValueError:
                pass
        return cp
    
    def summaryCopy(self, id=None, tag=None):
        '''  This method creates a copy of the node that can be used as a summary node. '''
        attrs = {}
        # Copy all of the attributes that are necessary for a summary node.
        for k in self.attrs.keys():
            try:
                if k.lower() in ['name', 'sum', 'num', 'type', 'units', 'slope', 'source']:
                    attrs[k.lower()] = self.attrs[k]
                attrs['sum'] = 0
                attrs['num'] = 0
            except ValueError:
                pass
        # Create a new node from the attributes that were copied from the existing node.
        cp = Element(self.id, attrs, tag)
        # Make sure that the summary node references the original children
        cp.children = self.children
        return cp
        
    def getAttr(self, attr):
        if self.attrs.has_key(attr.lower()):
            return self.attrs[attr.lower()]
        return None
        
    def getAttrs(self):
        return self.attrs

    def setAttr(self, attr, val):
        self.attrs[attr.lower()] = val
        
    def incAttr(self, attr, val):
        try:
            self.attrs[attr.lower()] += val
        except Exception, e:
            print 'Can not increment attribute ' + str(e)
        
    def getSummaryData(self):
        try:
            return self.summaryData
        except:
            return None
    
