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
    def generateKey(vals):
        if isinstance(vals,list):
            return ':'.join(vals)
        return vals
    generateKey = staticmethod(generateKey)
    
    def __init__(self, id, attrs, tag=None):
        self.id = id
        if tag is None:
            self.tag = id
        else:
            self.tag = tag
        for k,v in attrs.items():
            self.__dict__[k.lower()] = v
        self.children = {}
        
    def __setitem__(self, k, v):
        try:
            self.children[k].update(v)
        except KeyError:
            self.children[k] = v
            
    def __getitem__(self, k):
        return self.children[k]
        
    def update(self, elem):
        for k in self.__dict__.keys():
            if k == 'children' or k == 'id' or k == 'name':
                continue
            try:
                self.__dict__[k] = elem.__dict__[k]
            except ValueError:
                pass
        
    def __str__(self):
        if self.__dict__.has_key('name'):
            return Element.generateKey([self.id,self.name])
        return Element.generateKey(self.id)
        
    def __iter__(self):
        return self.children.itervalues()
        
    def __copy__(self):
        cp = Element(self.id, {})
        for k in self.__dict__.keys():
            if k == 'children':
                cp.children = {}
                continue
            try:
                cp.__dict__[k.lower()] = copy.copy(self.__dict__[k])
            except ValueError:
                pass
        return cp
    
    def summaryCopy(self, id=None, tag=None):
        attrs = {}
        for k in self.__dict__.keys():
            try:
                if k.lower() in ['name', 'sum', 'num', 'type', 'units', 'slop', 'source']:
                    attrs[k.lower()] = self.__dict__[k]
            except ValueError:
                pass
        cp = Element(self.id, attrs, tag)
        cp.children = self.children
        return cp
