
/*
 * BinaryFile over XMLHttpRequest
 * Part of the javascriptRRD package
 * Copyright (c) 2009 Frank Wuerthwein, fkw@ucsd.edu
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 *
 * Based on:
 *   Binary Ajax 0.1.5
 *   Copyright (c) 2008 Jacob Seidelin, cupboy@gmail.com, http://blog.nihilogic.dk/
 *   MIT License [http://www.opensource.org/licenses/mit-license.php]
 */

// ============================================================
// Exception class
function InvalidBinaryFile(msg) {
  this.message=msg;
  this.name="Invalid BinaryFile";
}

// pretty print
InvalidBinaryFile.prototype.toString = function() {
  return this.name + ': "' + this.message + '"';
}

// =====================================================================
// BinaryFile class
//   Allows access to element inside a binary stream
function BinaryFile(strData, iDataOffset, iDataLength) {
	var data = strData;
	var dataOffset = iDataOffset || 0;
	var dataLength = 0;
	// added 
	var doubleMantExpHi=Math.pow(2,-28);
	var doubleMantExpLo=Math.pow(2,-52);
	var doubleMantExpFast=Math.pow(2,-20);

	var switch_endian = false;

	this.getRawData = function() {
		return data;
	}

	if (typeof strData == "string") {
		dataLength = iDataLength || data.length;

		this.getByteAt = function(iOffset) {
			return data.charCodeAt(iOffset + dataOffset) & 0xFF;
		}
	} else if (typeof strData == "unknown") {
		dataLength = iDataLength || IEBinary_getLength(data);

		this.getByteAt = function(iOffset) {
			return IEBinary_getByteAt(data, iOffset + dataOffset);
		}
	} else {
	  throw new InvalidBinaryFile("Unsupported type " + (typeof strData));
	}

	this.getEndianByteAt = function(iOffset,width,delta) {
	  if (this.switch_endian) 
	    return this.getByteAt(iOffset+width-delta-1);
	  else
	    return this.getByteAt(iOffset+delta);
	}

	this.getLength = function() {
		return dataLength;
	}

	this.getSByteAt = function(iOffset) {
		var iByte = this.getByteAt(iOffset);
		if (iByte > 127)
			return iByte - 256;
		else
			return iByte;
	}

	this.getShortAt = function(iOffset) {
	        var iShort = (this.getEndianByteAt(iOffset,2,1) << 8) + this.getEndianByteAt(iOffset,2,0)
		if (iShort < 0) iShort += 65536;
		return iShort;
	}
	this.getSShortAt = function(iOffset) {
		var iUShort = this.getShortAt(iOffset);
		if (iUShort > 32767)
			return iUShort - 65536;
		else
			return iUShort;
	}
	this.getLongAt = function(iOffset) {
	        var iByte1 = this.getEndianByteAt(iOffset,4,0),
	             iByte2 = this.getEndianByteAt(iOffset,4,1),
	             iByte3 = this.getEndianByteAt(iOffset,4,2),
	             iByte4 = this.getEndianByteAt(iOffset,4,3);

		var iLong = (((((iByte4 << 8) + iByte3) << 8) + iByte2) << 8) + iByte1;
		if (iLong < 0) iLong += 4294967296;
		return iLong;
	}
	this.getSLongAt = function(iOffset) {
		var iULong = this.getLongAt(iOffset);
		if (iULong > 2147483647)
			return iULong - 4294967296;
		else
			return iULong;
	}
	this.getStringAt = function(iOffset, iLength) {
		var aStr = [];
		for (var i=iOffset,j=0;i<iOffset+iLength;i++,j++) {
			aStr[j] = String.fromCharCode(this.getByteAt(i));
		}
		return aStr.join("");
	}

	// Added
	this.getCStringAt = function(iOffset, iMaxLength) {
		var aStr = [];
		for (var i=iOffset,j=0;(i<iOffset+iMaxLength) && (this.getByteAt(i)>0);i++,j++) {
			aStr[j] = String.fromCharCode(this.getByteAt(i));
		}
		return aStr.join("");
	}

	// Added
	this.getDoubleAt = function(iOffset) {
	        var iByte1 = this.getEndianByteAt(iOffset,8,0),
	             iByte2 = this.getEndianByteAt(iOffset,8,1),
	             iByte3 = this.getEndianByteAt(iOffset,8,2),
	             iByte4 = this.getEndianByteAt(iOffset,8,3),
	             iByte5 = this.getEndianByteAt(iOffset,8,4),
	             iByte6 = this.getEndianByteAt(iOffset,8,5),
	             iByte7 = this.getEndianByteAt(iOffset,8,6),
	             iByte8 = this.getEndianByteAt(iOffset,8,7);
		var iSign=iByte8 >> 7;
		var iExpRaw=((iByte8 & 0x7F)<< 4) + (iByte7 >> 4);
		var iMantHi=((((((iByte7 & 0x0F) << 8) + iByte6) << 8) + iByte5) << 8) + iByte4;
		var iMantLo=((((iByte3) << 8) + iByte2) << 8) + iByte1;

		if (iExpRaw==0) return 0.0;
		if (iExpRaw==0x7ff) return undefined;

		var iExp=(iExpRaw & 0x7FF)-1023;

		var dDouble = ((iSign==1)?-1:1)*Math.pow(2,iExp)*(1.0 + iMantLo*doubleMantExpLo + iMantHi*doubleMantExpHi);
		return dDouble;
	}
	// added
	// Extracts only 4 bytes out of 8, loosing in precision (20 bit mantissa)
	this.getFastDoubleAt = function(iOffset) {
	        var iByte5 = this.getEndianByteAt(iOffset,8,4),
		     iByte6 = this.getEndianByteAt(iOffset,8,5),
		     iByte7 = this.getEndianByteAt(iOffset,8,6),
		     iByte8 = this.getEndianByteAt(iOffset,8,7);
		var iSign=iByte8 >> 7;
		var iExpRaw=((iByte8 & 0x7F)<< 4) + (iByte7 >> 4);
		var iMant=((((iByte7 & 0x0F) << 8) + iByte6) << 8) + iByte5;

		if (iExpRaw==0) return 0.0;
		if (iExpRaw==0x7ff) return undefined;

		var iExp=(iExpRaw & 0x7FF)-1023;

		var dDouble = ((iSign==1)?-1:1)*Math.pow(2,iExp)*(1.0 + iMant*doubleMantExpFast);
		return dDouble;
	}

	this.getCharAt = function(iOffset) {
		return String.fromCharCode(this.getByteAt(iOffset));
	}
}


document.write(
	"<script type='text/vbscript'>\r\n"
	+ "Function IEBinary_getByteAt(strBinary, iOffset)\r\n"
	+ "	IEBinary_getByteAt = AscB(MidB(strBinary,iOffset+1,1))\r\n"
	+ "End Function\r\n"
	+ "Function IEBinary_getLength(strBinary)\r\n"
	+ "	IEBinary_getLength = LenB(strBinary)\r\n"
	+ "End Function\r\n"
	+ "</script>\r\n"
);



// ===============================================================
// Load a binary file from the specified URL 
// Will return an object of type BinaryFile
function FetchBinaryURL(url) {
  var request =  new XMLHttpRequest();
  request.open("GET", url,false);
  try {
    request.overrideMimeType('text/plain; charset=x-user-defined');
  } catch (err) {
    // ignore any error, just to make both FF and IE work
  }
  request.send(null);

  var response=this.responseText;
  try {
    // for older IE versions, the value in responseText is not usable
    if (IEBinary_getLength(this.responseBody)>0) {
      // will get here only for older verson of IE
      response=this.responseBody;
    }
  } catch (err) {
    // not IE, do nothing
  }

  var bf=new BinaryFile(response);
  return bf;
}


// ===============================================================
// Asyncronously load a binary file from the specified URL 
//
// callback must be a function with one or two arguments:
//  - bf = an object of type BinaryFile
//  - optional argument object (used only if callback_arg not undefined) 
function FetchBinaryURLAsync(url, callback, callback_arg) {
  var callback_wrapper = function() {
    if(this.readyState == 4) {
      var response=this.responseText;
      try {
        // for older IE versions, the value in responseText is not usable
        if (IEBinary_getLength(this.responseBody)>0) {
          // will get here only for older verson of IE
          response=this.responseBody;
        }
      } catch (err) {
       // not IE, do nothing
      }

      var bf=new BinaryFile(response);
      if (callback_arg!=null) {
	callback(bf,callback_arg);
      } else {
	callback(bf);
      }
    }
  }

  var request =  new XMLHttpRequest();
  request.onreadystatechange = callback_wrapper;
  request.open("GET", url,true);
  try {
    request.overrideMimeType('text/plain; charset=x-user-defined');
  } catch (err) {
    // ignore any error, just to make both FF and IE work
  }
  request.send(null);
  return request
}
/*
 * Client library for access to RRD archive files
 * Part of the javascriptRRD package
 * Copyright (c) 2009-2010 Frank Wuerthwein, fkw@ucsd.edu
 *                         Igor Sfiligoi, isfiligoi@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

/*
 *
 * RRDTool has been developed and is maintained by
 * Tobias Oether [http://oss.oetiker.ch/rrdtool/]
 *
 * This software can be used to read files produced by the RRDTool
 * but has been developed independently.
 * 
 * Limitations:
 *
 * This version of the module assumes RRD files created on linux 
 * with intel architecture and supports both 32 and 64 bit CPUs.
 * All integers in RRD files are suppoes to fit in 32bit values.
 *
 * Only versions 3 and 4 of the RRD archive are supported.
 *
 * Only AVERAGE,MAXIMUM,MINIMUM and LAST consolidation functions are
 * supported. For all others, the behaviour is at the moment undefined.
 *
 */

/*
 * Dependencies:
 *   
 * The data provided to this module require an object of a class
 * that implements the following methods:
 *   getByteAt(idx)            - Return a 8 bit unsigned integer at offset idx
 *   getLongAt(idx)            - Return a 32 bit unsigned integer at offset idx
 *   getDoubleAt(idx)          - Return a double float at offset idx
 *   getFastDoubleAt(idx)      - Similar to getDoubleAt but with less precision
 *   getCStringAt(idx,maxsize) - Return a string of at most maxsize characters
 *                               that was 0-terminated in the source
 *
 * The BinaryFile from binaryXHR.js implements this interface.
 *
 */


// ============================================================
// Exception class
function InvalidRRD(msg) {
  this.message=msg;
  this.name="Invalid RRD";
}

// pretty print
InvalidRRD.prototype.toString = function() {
  return this.name + ': "' + this.message + '"';
}


// ============================================================
// RRD DS Info class
function RRDDS(rrd_data,rrd_data_idx,my_idx) {
  this.rrd_data=rrd_data;
  this.rrd_data_idx=rrd_data_idx;
  this.my_idx=my_idx;
}

RRDDS.prototype.getIdx = function() {
  return this.my_idx;
}
RRDDS.prototype.getName = function() {
  return this.rrd_data.getCStringAt(this.rrd_data_idx,20);
}
RRDDS.prototype.getType = function() {
  return this.rrd_data.getCStringAt(this.rrd_data_idx+20,20);
}
RRDDS.prototype.getMin = function() {
  return this.rrd_data.getDoubleAt(this.rrd_data_idx+48);
}
RRDDS.prototype.getMax = function() {
  return this.rrd_data.getDoubleAt(this.rrd_data_idx+56);
}


// ============================================================
// RRD RRA Info class
function RRDRRAInfo(rrd_data,rra_def_idx,
		    int_align,row_cnt,pdp_step,my_idx) {
  this.rrd_data=rrd_data;
  this.rra_def_idx=rra_def_idx;
  this.int_align=int_align;
  this.row_cnt=row_cnt;
  this.pdp_step=pdp_step;
  this.my_idx=my_idx;

  // char nam[20], uint row_cnt, uint pdp_cnt
  this.rra_pdp_cnt_idx=rra_def_idx+Math.ceil(20/int_align)*int_align+int_align;
}

RRDRRAInfo.prototype.getIdx = function() {
  return this.my_idx;
}

// Get number of rows
RRDRRAInfo.prototype.getNrRows = function() {
  return this.row_cnt;
}

// Get number of slots used for consolidation
// Mostly for internal use
RRDRRAInfo.prototype.getPdpPerRow = function() {
  return this.rrd_data.getLongAt(this.rra_pdp_cnt_idx);
}

// Get RRA step (expressed in seconds)
RRDRRAInfo.prototype.getStep = function() {
  return this.pdp_step*this.getPdpPerRow();
}

// Get consolidation function name
RRDRRAInfo.prototype.getCFName = function() {
  return this.rrd_data.getCStringAt(this.rra_def_idx,20);
}


// ============================================================
// RRD RRA handling class
function RRDRRA(rrd_data,rra_ptr_idx,
		rra_info,
		header_size,prev_row_cnts,ds_cnt) {
  this.rrd_data=rrd_data;
  this.rra_info=rra_info;
  this.row_cnt=rra_info.row_cnt;
  this.ds_cnt=ds_cnt;

  var row_size=ds_cnt*8;

  this.base_rrd_db_idx=header_size+prev_row_cnts*row_size;

  // get imediately, since it will be needed often
  this.cur_row=rrd_data.getLongAt(rra_ptr_idx);

  // calculate idx relative to base_rrd_db_idx
  // mostly used internally
  this.calc_idx = function(row_idx,ds_idx) {
    if ((row_idx>=0) && (row_idx<this.row_cnt)) {
      if ((ds_idx>=0) && (ds_idx<ds_cnt)){
	// it is round robin, starting from cur_row+1
	var real_row_idx=row_idx+this.cur_row+1;
	if (real_row_idx>=this.row_cnt) real_row_idx-=this.row_cnt;
	return row_size*real_row_idx+ds_idx*8;
      } else {
	throw RangeError("DS idx ("+ row_idx +") out of range [0-" + ds_cnt +").");
      }
    } else {
      throw RangeError("Row idx ("+ row_idx +") out of range [0-" + this.row_cnt +").");
    }	
  }
}

RRDRRA.prototype.getIdx = function() {
  return this.rra_info.getIdx();
}

// Get number of rows/columns
RRDRRA.prototype.getNrRows = function() {
  return this.row_cnt;
}
RRDRRA.prototype.getNrDSs = function() {
  return this.ds_cnt;
}

// Get RRA step (expressed in seconds)
RRDRRA.prototype.getStep = function() {
  return this.rra_info.getStep();
}

// Get consolidation function name
RRDRRA.prototype.getCFName = function() {
  return this.rra_info.getCFName();
}

RRDRRA.prototype.getEl = function(row_idx,ds_idx) {
  return this.rrd_data.getDoubleAt(this.base_rrd_db_idx+this.calc_idx(row_idx,ds_idx));
}

// Low precision version of getEl
// Uses getFastDoubleAt
RRDRRA.prototype.getElFast = function(row_idx,ds_idx) {
  return this.rrd_data.getFastDoubleAt(this.base_rrd_db_idx+this.calc_idx(row_idx,ds_idx));
}

// ============================================================
// RRD Header handling class
function RRDHeader(rrd_data) {
  this.rrd_data=rrd_data;
  this.validate_rrd();
  this.calc_idxs();
}

// Internal, used for initialization
RRDHeader.prototype.validate_rrd = function() {
  if (this.rrd_data.getLength()<1) throw new InvalidRRD("Empty file.");
  if (this.rrd_data.getLength()<16) throw new InvalidRRD("File too short.");
  if (this.rrd_data.getCStringAt(0,4)!=="RRD") throw new InvalidRRD("Wrong magic id.");

  this.rrd_version=this.rrd_data.getCStringAt(4,5);
  if ((this.rrd_version!=="0003")&&(this.rrd_version!=="0004")&&(this.rrd_version!=="0001")) {
    throw new InvalidRRD("Unsupported RRD version "+this.rrd_version+".");
  }

  this.float_width=8;
  if (this.rrd_data.getLongAt(12)==0) {
    // not a double here... likely 64 bit
    this.float_align=8;
    if (! (this.rrd_data.getDoubleAt(16)==8.642135e+130)) {
      // uhm... wrong endian?
      this.rrd_data.switch_endian=true;
    }
    if (this.rrd_data.getDoubleAt(16)==8.642135e+130) {
      // now, is it all 64bit or only float 64 bit?
      if (this.rrd_data.getLongAt(28)==0) {
	// true 64 bit align
	this.int_align=8;
	this.int_width=8;
      } else {
	// integers are 32bit aligned
	this.int_align=4;
	this.int_width=4;
      }
    } else {
      throw new InvalidRRD("Magic float not found at 16.");
    }
  } else {
    /// should be 32 bit alignment
    if (! (this.rrd_data.getDoubleAt(12)==8.642135e+130)) {
      // uhm... wrong endian?
      this.rrd_data.switch_endian=true;
    }
    if (this.rrd_data.getDoubleAt(12)==8.642135e+130) {
      this.float_align=4;
      this.int_align=4;
      this.int_width=4;
    } else {
      throw new InvalidRRD("Magic float not found at 12.");
    }
  }
  this.unival_width=this.float_width;
  this.unival_align=this.float_align;

  // process the header here, since I need it for validation

  // char magic[4], char version[5], double magic_float

  // long ds_cnt, long rra_cnt, long pdp_step, unival par[10]
  this.ds_cnt_idx=Math.ceil((4+5)/this.float_align)*this.float_align+this.float_width;
  this.rra_cnt_idx=this.ds_cnt_idx+this.int_width;
  this.pdp_step_idx=this.rra_cnt_idx+this.int_width;

  //always get only the low 32 bits, the high 32 on 64 bit archs should always be 0
  this.ds_cnt=this.rrd_data.getLongAt(this.ds_cnt_idx);
  if (this.ds_cnt<1) {
    throw new InvalidRRD("ds count less than 1.");
  }

  this.rra_cnt=this.rrd_data.getLongAt(this.rra_cnt_idx);
  if (this.ds_cnt<1) {
    throw new InvalidRRD("rra count less than 1.");
  }

  this.pdp_step=this.rrd_data.getLongAt(this.pdp_step_idx);
  if (this.pdp_step<1) {
    throw new InvalidRRD("pdp step less than 1.");
  }

  // best guess, assuming no weird align problems
  this.top_header_size=Math.ceil((this.pdp_step_idx+this.int_width)/this.unival_align)*this.unival_align+10*this.unival_width;
  var t=this.rrd_data.getLongAt(this.top_header_size);
  if (t==0) {
    throw new InvalidRRD("Could not find first DS name.");
  }
}

// Internal, used for initialization
RRDHeader.prototype.calc_idxs = function() {
  this.ds_def_idx=this.top_header_size;
  // char ds_nam[20], char dst[20], unival par[10]
  this.ds_el_size=Math.ceil((20+20)/this.unival_align)*this.unival_align+10*this.unival_width;

  this.rra_def_idx=this.ds_def_idx+this.ds_el_size*this.ds_cnt;
  // char cf_nam[20], uint row_cnt, uint pdp_cnt, unival par[10]
  this.row_cnt_idx=Math.ceil(20/this.int_align)*this.int_align;
  this.rra_def_el_size=Math.ceil((this.row_cnt_idx+2*this.int_width)/this.unival_align)*this.unival_align+10*this.unival_width;

  this.live_head_idx=this.rra_def_idx+this.rra_def_el_size*this.rra_cnt;
  // time_t last_up, int last_up_usec
  this.live_head_size=2*this.int_width;

  this.pdp_prep_idx=this.live_head_idx+this.live_head_size;
  // char last_ds[30], unival scratch[10]
  this.pdp_prep_el_size=Math.ceil(30/this.unival_align)*this.unival_align+10*this.unival_width;

  this.cdp_prep_idx=this.pdp_prep_idx+this.pdp_prep_el_size*this.ds_cnt;
  // unival scratch[10]
  this.cdp_prep_el_size=10*this.unival_width;

  this.rra_ptr_idx=this.cdp_prep_idx+this.cdp_prep_el_size*this.ds_cnt*this.rra_cnt;
  // uint cur_row
  this.rra_ptr_el_size=1*this.int_width;
  
  this.header_size=this.rra_ptr_idx+this.rra_ptr_el_size*this.rra_cnt;
}

// Optional initialization
// Read and calculate row counts
RRDHeader.prototype.load_row_cnts = function() {
  this.rra_def_row_cnts=[];
  this.rra_def_row_cnt_sums=[]; // how many rows before me
  for (var i=0; i<this.rra_cnt; i++) {
    this.rra_def_row_cnts[i]=this.rrd_data.getLongAt(this.rra_def_idx+i*this.rra_def_el_size+this.row_cnt_idx,false);
    if (i==0) {
      this.rra_def_row_cnt_sums[i]=0;
    } else {
      this.rra_def_row_cnt_sums[i]=this.rra_def_row_cnt_sums[i-1]+this.rra_def_row_cnts[i-1];
    }
  }
}

// ---------------------------
// Start of user functions

RRDHeader.prototype.getMinStep = function() {
  return this.pdp_step;
}
RRDHeader.prototype.getLastUpdate = function() {
  return this.rrd_data.getLongAt(this.live_head_idx,false);
}

RRDHeader.prototype.getNrDSs = function() {
  return this.ds_cnt;
}
RRDHeader.prototype.getDSNames = function() {
  var ds_names=[]
  for (var idx=0; idx<this.ds_cnt; idx++) {
    var ds=this.getDSbyIdx(idx);
    var ds_name=ds.getName()
    ds_names.push(ds_name);
  }
  return ds_names;
}
RRDHeader.prototype.getDSbyIdx = function(idx) {
  if ((idx>=0) && (idx<this.ds_cnt)) {
    return new RRDDS(this.rrd_data,this.ds_def_idx+this.ds_el_size*idx,idx);
  } else {
    throw RangeError("DS idx ("+ idx +") out of range [0-" + this.ds_cnt +").");
  }	
}
RRDHeader.prototype.getDSbyName = function(name) {
  for (var idx=0; idx<this.ds_cnt; idx++) {
    var ds=this.getDSbyIdx(idx);
    var ds_name=ds.getName()
    if (ds_name==name)
      return ds;
  }
  throw RangeError("DS name "+ name +" unknown.");
}

RRDHeader.prototype.getNrRRAs = function() {
  return this.rra_cnt;
}
RRDHeader.prototype.getRRAInfo = function(idx) {
  if ((idx>=0) && (idx<this.rra_cnt)) {
    return new RRDRRAInfo(this.rrd_data,
			  this.rra_def_idx+idx*this.rra_def_el_size,
			  this.int_align,this.rra_def_row_cnts[idx],this.pdp_step,
			  idx);
  } else {
    throw RangeError("RRA idx ("+ idx +") out of range [0-" + this.rra_cnt +").");
  }	
}

// ============================================================
// RRDFile class
//   Given a BinaryFile, gives access to the RRD archive fields
// 
// Arguments:
//   bf must be an object compatible with the BinaryFile interface
//   file_options - currently no semantics... introduced for future expandability
function RRDFile(bf,file_options) {
  this.file_options=file_options;

  var rrd_data=bf

  this.rrd_header=new RRDHeader(rrd_data);
  this.rrd_header.load_row_cnts();

  // ===================================
  // Start of user functions

  this.getMinStep = function() {
    return this.rrd_header.getMinStep();
  }
  this.getLastUpdate = function() {
    return this.rrd_header.getLastUpdate();
  }

  this.getNrDSs = function() {
    return this.rrd_header.getNrDSs();
  }
  this.getDSNames = function() {
    return this.rrd_header.getDSNames();
  }
  this.getDS = function(id) {
    if (typeof id == "number") {
      return this.rrd_header.getDSbyIdx(id);
    } else {
      return this.rrd_header.getDSbyName(id);
    }
  }

  this.getNrRRAs = function() {
    return this.rrd_header.getNrRRAs();
  }

  this.getRRAInfo = function(idx) {
    return this.rrd_header.getRRAInfo(idx);
  }

  this.getRRA = function(idx) {
    rra_info=this.rrd_header.getRRAInfo(idx);
    return new RRDRRA(rrd_data,
		      this.rrd_header.rra_ptr_idx+idx*this.rrd_header.rra_ptr_el_size,
		      rra_info,
		      this.rrd_header.header_size,
		      this.rrd_header.rra_def_row_cnt_sums[idx],
		      this.rrd_header.ds_cnt);
  }

}
/*
 * Support library aimed at providing commonly used functions and classes
 * that may be used while plotting RRD files with Flot
 *
 * Part of the javascriptRRD package
 * Copyright (c) 2009 Frank Wuerthwein, fkw@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

/*
 *
 * Flot is a javascript plotting library developed and maintained by
 * Ole Laursen [http://www.flotcharts.org/]
 *
 */

// Return a Flot-like data structure
// Since Flot does not properly handle empty elements, min and max are returned, too
function rrdDS2FlotSeries(rrd_file,ds_id,rra_idx,want_rounding) {
  var ds=rrd_file.getDS(ds_id);
  var ds_name=ds.getName();
  var ds_idx=ds.getIdx();
  var rra=rrd_file.getRRA(rra_idx);
  var rra_rows=rra.getNrRows();
  var last_update=rrd_file.getLastUpdate();
  var step=rra.getStep();

  if (want_rounding!=false) {
    // round last_update to step
    // so that all elements are sync
    last_update-=(last_update%step); 
  }

  var first_el=last_update-(rra_rows-1)*step;
  var timestamp=first_el;
  var flot_series=[];
  for (var i=0;i<rra_rows;i++) {
    var el=rra.getEl(i,ds_idx);
    if (el!=undefined) {
      flot_series.push([timestamp*1000.0,el]);
    }
    timestamp+=step;
  } // end for

  return {label: ds_name, data: flot_series, min: first_el*1000.0, max:last_update*1000.0};
}

// return an object with an array containing Flot elements, one per DS
// min and max are also returned
function rrdRRA2FlotObj(rrd_file,rra_idx,ds_list,want_ds_labels,want_rounding) {
  var rra=rrd_file.getRRA(rra_idx);
  var rra_rows=rra.getNrRows();
  var last_update=rrd_file.getLastUpdate();
  var step=rra.getStep();
  if (want_rounding!=false) {
    // round last_update to step
    // so that all elements are sync
    last_update-=(last_update%step); 
  }

  var first_el=last_update-(rra_rows-1)*step;

  var out_el={data:[], min:first_el*1000.0, max:last_update*1000.0};

  var ds_list_len = ds_list.length;
  for (var ds_list_idx=0; ds_list_idx<ds_list_len; ++ds_list_idx) { 
    var ds_id=ds_list[ds_list_idx];
    var ds=rrd_file.getDS(ds_id);
    var ds_name=ds.getName();
    var ds_idx=ds.getIdx();

    var timestamp=first_el;
    var flot_series=[];
    for (var i=0;i<rra_rows;i++) {
      var el=rra.getEl(i,ds_idx);
      if (el!=undefined) {
	flot_series.push([timestamp*1000.0,el]);
      }
      timestamp+=step;
    } // end for
    
    var flot_el={data:flot_series};
    if (want_ds_labels!=false) {
      var ds_name=ds.getName();
      flot_el.label= ds_name;
    }
    out_el.data.push(flot_el);
  } //end for ds_list_idx
  return out_el;
}

// return an object with an array containing Flot elements
//  have a positive and a negative stack of DSes, plus DSes with no stacking
// min and max are also returned
// If one_undefined_enough==true, a whole stack is invalidated if a single element
//  of the stack is invalid
function rrdRRAStackFlotObj(rrd_file,rra_idx,
			    ds_positive_stack_list,ds_negative_stack_list,ds_single_list,
                            timestamp_shift, want_ds_labels,want_rounding,one_undefined_enough) {
  var rra=rrd_file.getRRA(rra_idx);
  var rra_rows=rra.getNrRows();
  var last_update=rrd_file.getLastUpdate();
  var step=rra.getStep();
  if (want_rounding!=false) {
    // round last_update to step
    // so that all elements are sync
    last_update-=(last_update%step); 
  }
  if (one_undefined_enough!=true) { // make sure it is a boolean
    one_undefined_enough=false;
  }

  var first_el=last_update-(rra_rows-1)*step;

  var out_el={data:[], min:(first_el+timestamp_shift)*1000.0, max:(last_update+timestamp_shift)*1000.0};

  // first the stacks stack
  var stack_els=[ds_positive_stack_list,ds_negative_stack_list];
  var stack_els_len = stack_els.length;
  for (var stack_list_id=0; stack_list_id<stack_els_len; ++stack_list_id) {
    var stack_list=stack_els[stack_list_id];
    var tmp_flot_els=[];
    var tmp_ds_ids=[];
    var tmp_nr_ids=stack_list.length;
    var stack_list_len = stack_list.length;
    for (var ds_list_idx=0; ds_list_idx<stack_list_len; ++ds_list_idx) {
      var ds_id=stack_list[ds_list_idx];
      var ds=rrd_file.getDS(ds_id);
      var ds_name=ds.getName();
      var ds_idx=ds.getIdx();
      tmp_ds_ids.push(ds_idx); // getting this is expensive, call only once
      
      // initialize
      var flot_el={data:[]}
      if (want_ds_labels!=false) {
	var ds_name=ds.getName();
	flot_el.label= ds_name;
      }
      tmp_flot_els.push(flot_el);
    }

    var timestamp=first_el;
    for (var row=0;row<rra_rows;row++) {
      var ds_vals=[];
      var all_undef=true;
      var all_def=true;
      for (var id=0; id<tmp_nr_ids; id++) {
	var ds_idx=tmp_ds_ids[id];
	var el=rra.getEl(row,ds_idx);
	if (el!=undefined) {
	  all_undef=false;
	  ds_vals.push(el);
	} else {
	  all_def=false;
	  ds_vals.push(0);
	}
      } // end for id
      if (!all_undef) { // if all undefined, skip
	if (all_def || (!one_undefined_enough)) {
	  // this is a valid column, do the math
	  for (var id=1; id<tmp_nr_ids; id++) {
	    ds_vals[id]+=ds_vals[id-1]; // both positive and negative stack use a +, negative stack assumes negative values
	  }
	  // fill the flot data
	  for (var id=0; id<tmp_nr_ids; id++) {
	    tmp_flot_els[id].data.push([(timestamp+timestamp_shift)*1000.0,ds_vals[id]]);
	  }
	}
      } // end if

      timestamp+=step;
    } // end for row
    
    // put flot data in output object
    // reverse order so higher numbers are behind
    for (var id=0; id<tmp_nr_ids; id++) {
      out_el.data.push(tmp_flot_els[tmp_nr_ids-id-1]);
    }
  } //end for stack_list_id

  var ds_single_list_len = ds_single_list.length;
  for (var ds_list_idx=0; ds_list_idx<ds_single_list_len; ++ds_list_idx) { 
    var ds_id=ds_single_list[ds_list_idx];
    var ds=rrd_file.getDS(ds_id);
    var ds_name=ds.getName();
    var ds_idx=ds.getIdx();

    var timestamp=first_el;
    var flot_series=[];
    for (var i=0;i<rra_rows;i++) {
      var el=rra.getEl(i,ds_idx);
      if (el!=undefined) {
	flot_series.push([(timestamp+timestamp_shift)*1000.0,el]);
      }
      timestamp+=step;
    } // end for
    
    var flot_el={data:flot_series};
    if (want_ds_labels!=false) {
      var ds_name=ds.getName();
      flot_el.label= ds_name;
    }
    out_el.data.push(flot_el);
  } //end for ds_list_idx

  return out_el;
}

// return an object with an array containing Flot elements, one per RRD
// min and max are also returned
function rrdRRAMultiStackFlotObj(rrd_files, // a list of [rrd_id,rrd_file] pairs, all rrds must have the same step
				 rra_idx,ds_id,
				 want_rrd_labels,want_rounding,
				 one_undefined_enough) { // If true, a whole stack is invalidated if a single element of the stack is invalid

  var reference_rra=rrd_files[0][1].getRRA(rra_idx); // get the first one, all should be the same
  var rows=reference_rra.getNrRows();
  var step=reference_rra.getStep();
  var ds_idx=rrd_files[0][1].getDS(ds_id).getIdx(); // this can be expensive, do once (all the same)

  // rrds can be slightly shifted, calculate range
  var max_ts=null;
  var min_ts=null;

  // initialize list of rrd data elements
  var tmp_flot_els=[];
  var tmp_rras=[];
  var tmp_last_updates=[];
  var tmp_nr_ids=rrd_files.length;
  for (var id=0; id<tmp_nr_ids; id++) {
    var rrd_file=rrd_files[id][1];
    var rrd_rra=rrd_file.getRRA(rra_idx);

    var rrd_last_update=rrd_file.getLastUpdate();
    if (want_rounding!=false) {
      // round last_update to step
      // so that all elements are sync
      rrd_last_update-=(rrd_last_update%step); 
    }
    tmp_last_updates.push(rrd_last_update);

    var rrd_min_ts=rrd_last_update-(rows-1)*step;
    if ((max_ts==null) || (rrd_last_update>max_ts)) {
      max_ts=rrd_last_update;
    }
    if ((min_ts==null) || (rrd_min_ts<min_ts)) {
      min_ts=rrd_min_ts;
    }
    
    tmp_rras.push(rrd_rra);
      
    // initialize
    var flot_el={data:[]}
    if (want_rrd_labels!=false) {
	var rrd_name=rrd_files[id][0];
	flot_el.label= rrd_name;
    }
    tmp_flot_els.push(flot_el);
  }

  var out_el={data:[], min:min_ts*1000.0, max:max_ts*1000.0};

  for (var ts=min_ts;ts<=max_ts;ts+=step) {
      var rrd_vals=[];
      var all_undef=true;
      var all_def=true;
      for (var id=0; id<tmp_nr_ids; id++) {
        var rrd_rra=tmp_rras[id];
        var rrd_last_update=tmp_last_updates[id];
	var row_delta=Math.round((rrd_last_update-ts)/step);
	var el=undefined; // if out of range
        if ((row_delta>=0) && (row_delta<rows)) {
          el=rrd_rra.getEl(rows-row_delta-1,ds_idx);
        }
	if (el!=undefined) {
	  all_undef=false;
	  rrd_vals.push(el);
	} else {
	  all_def=false;
	  rrd_vals.push(0);
	}
      } // end for id
      if (!all_undef) { // if all undefined, skip
	if (all_def || (!one_undefined_enough)) {
	  // this is a valid column, do the math
	  for (var id=1; id<tmp_nr_ids; id++) {
	    rrd_vals[id]+=rrd_vals[id-1]; 
	  }
	  // fill the flot data
	  for (var id=0; id<tmp_nr_ids; id++) {
	    tmp_flot_els[id].data.push([ts*1000.0,rrd_vals[id]]);
	  }
	}
      } // end if
  } // end for ts
    
  // put flot data in output object
  // reverse order so higher numbers are behind
  for (var id=0; id<tmp_nr_ids; id++) {
    out_el.data.push(tmp_flot_els[tmp_nr_ids-id-1]);
  }
  
  return out_el;
}

// ======================================
// Helper class for handling selections
// =======================================================
function rrdFlotSelection() {
  this.selection_min=null;
  this.selection_max=null;
};

// reset to a state where ther is no selection
rrdFlotSelection.prototype.reset = function() {
  this.selection_min=null;
  this.selection_max=null;
};

// given the selection ranges, set internal variable accordingly
rrdFlotSelection.prototype.setFromFlotRanges = function(ranges) {
  this.selection_min=ranges.xaxis.from;
  this.selection_max=ranges.xaxis.to;
};

// Return a Flot ranges structure that can be promptly used in setSelection
rrdFlotSelection.prototype.getFlotRanges = function() {
  return { xaxis: {from: this.selection_min, to: this.selection_max}};
};

// return true is a selection is in use
rrdFlotSelection.prototype.isSet = function() {
  return this.selection_min!=null;
};

// Given an array of flot lines, limit to the selection
rrdFlotSelection.prototype.trim_flot_data = function(flot_data) {
  var out_data=[];
  for (var i=0; i<flot_data.length; i++) {
    var data_el=flot_data[i];
    out_data.push({label : data_el.label, data:this.trim_data(data_el.data), color:data_el.color, lines:data_el.lines, yaxis:data_el.yaxis});
  }
  return out_data;
};

// Limit to selection the flot series data element
rrdFlotSelection.prototype.trim_data = function(data_list) {
  if (this.selection_min==null) return data_list; // no selection => no filtering

  var out_data=[];
  for (var i=0; i<data_list.length; i++) {
    
    if (data_list[i]==null) continue; // protect
    //data_list[i][0]+=3550000*5;
    var nr=data_list[i][0]; //date in unix time
    if ((nr>=this.selection_min) && (nr<=this.selection_max)) {
      out_data.push(data_list[i]);
    }
  }
  return out_data;
};


// Given an array of flot lines, limit to the selection
rrdFlotSelection.prototype.trim_flot_timezone_data = function(flot_data,shift) {
  var out_data=[];
  for (var i=0; i<flot_data.length; i++) {
    var data_el=flot_data[i];
    out_data.push({label : data_el.label, data:this.trim_timezone_data(data_el.data,shift), color:data_el.color, lines:data_el.lines, yaxis:data_el.yaxis});
  }
  return out_data;
};

// Limit to selection the flot series data element
rrdFlotSelection.prototype.trim_timezone_data = function(data_list,shift) {
  if (this.selection_min==null) return data_list; // no selection => no filtering

  var out_data=[];
  for (var i=0; i<data_list.length; i++) {
    if (data_list[i]==null) continue; // protect
    var nr=data_list[i][0]+shift;
    if ((nr>=this.selection_min) && (nr<=this.selection_max)) {
      out_data.push(data_list[i]);
    }
  }
  return out_data;
};


// ======================================
// Miscelaneous helper functions
// ======================================

function rfs_format_time(s) {
  if (s<120) {
    return s+"s";
  } else {
    var s60=s%60;
    var m=(s-s60)/60;
    if ((m<10) && (s60>9)) {
      return m+":"+s60+"min";
    } if (m<120) {
      return m+"min";
    } else {
      var m60=m%60;
      var h=(m-m60)/60;
      if ((h<12) && (m60>9)) {
	return h+":"+m60+"h";
      } if (h<48) {
	return h+"h";
      } else {
	var h24=h%24;
	var d=(h-h24)/24;
	if ((d<7) && (h24>0)) {
	  return d+" days "+h24+"h";
	} if (d<60) {
	  return d+" days";
	} else {
	  var d30=d%30;
	  var mt=(d-d30)/30;
	  return mt+" months";
	}
      }
    }
    
  }
}
/*
 * RRD graphing libraries, based on Flot
 * Part of the javascriptRRD package
 * Copyright (c) 2010 Frank Wuerthwein, fkw@ucsd.edu
 *                    Igor Sfiligoi, isfiligoi@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

/*
 *
 * Flot is a javascript plotting library developed and maintained by
 * Ole Laursen [http://code.google.com/p/flot/]
 *
 */

/*
 * Local dependencies:
 *  rrdFlotSupport.py
 *
 * External dependencies:
 *  [Flot]/jquery.py
 *  [Flot]/jquery.flot.js
 *  [Flot]/jquery.flot.selection.js
 */

/* graph_options defaults (see Flot docs for details)
 * {
 *  legend: { position:"nw",noColumns:3},
 *  lines: { show:true },
 *  yaxis: { autoscaleMargin: 0.20},
 *  tooltip: true,
 *  tooltipOpts: { content: "<h4>%s</h4> Value: %y.3" }
 * }
 *
 * ds_graph_options is a dictionary of DS_name, 
 *   with each element being a graph_option
 *   The defaults for each element are
 *   {
 *     title: label  or ds_name     // this is what is displayed in the checkboxes
 *     checked: first_ds_in_list?   // boolean
 *     label: title or ds_name      // this is what is displayed in the legend
 *     color: ds_index              // see Flot docs for details
 *     lines: { show:true }         // see Flot docs for details
 *     yaxis: 1                     // can be 1 or 2
 *     stack: 'none'                // other options are 'positive' and 'negative'
 *   }
 *
 * //overwrites other defaults; mostly used for linking via the URL
 * rrdflot_defaults defaults (see Flot docs for details) 	 
 * {
 *    graph_only: false        // If true, limit the display to the graph only
 *    legend: "Top"            //Starting location of legend. Options are: 
 *                             //   "Top","Bottom","TopRight","BottomRight","None".
 *    num_cb_rows: 12          //How many rows of DS checkboxes per column.
 *    use_element_buttons: false  //To be used in conjunction with num_cb_rows: This option
 *                             //    creates a button above every column, which selects
 *                             //    every element in the column. 
 *    multi_ds: false          //"true" appends the name of the aggregation function to the 
 *                             //    name of the DS element. 
 *    multi_rra: false         //"true" appends the name of the RRA consolidation function (CF) 
 *                             //    (AVERAGE, MIN, MAX or LAST) to the name of the RRA. Useful 
 *                             //    for RRAs over the same interval with different CFs.  
 *    use_checked_DSs: false   //Use the list checked_DSs below.
 *    checked_DSs: []          //List of elements to be checked by default when graph is loaded. 
 *                             //    Overwrites graph options. 
 *    use_rra: false           //Whether to use the rra index specified below.
 *    rra: 0                   //RRA (rra index in rrd) to be selected when graph is loaded. 
 *    use_windows: false       //Whether to use the window zoom specifications below.
 *    window_min: 0            //Sets minimum for window zoom. X-axis usually in unix time. 
 *    window_max: 0            //Sets maximum for window zoom.
 *    graph_height: "300px"    //Height of main graph. 
 *    graph_width: "500px"     //Width of main graph.
 *    scale_height: "110px"    //Height of small scaler graph.
 *    scale_width: "250px"     //Width of small scaler graph.
 *    timezone: local          //timezone.
 * } 
 */

var local_checked_DSs = [];
var selected_rra = 0;
var window_min=0;
var window_max=0;
var elem_group=null;
var timezone_shift=0;

function rrdFlot(html_id, rrd_file, graph_options, ds_graph_options, rrdflot_defaults) {
  this.html_id=html_id;
  this.rrd_file=rrd_file;
  this.graph_options=graph_options;
  if (rrdflot_defaults==null) {
    this.rrdflot_defaults=new Object(); // empty object, just not to be null
  } else {
    this.rrdflot_defaults=rrdflot_defaults;
  }
  if (ds_graph_options==null) {
    this.ds_graph_options=new Object(); // empty object, just not to be null
  } else {
    this.ds_graph_options=ds_graph_options;
  }
  this.selection_range=new rrdFlotSelection();

  graph_info={};
  this.createHTML();
  this.populateRes();
  this.populateDScb();
  this.drawFlotGraph();

  if (this.rrdflot_defaults.graph_only==true) {
    this.cleanHTMLCruft();
  }
}


// ===============================================
// Create the HTML tags needed to host the graphs
rrdFlot.prototype.createHTML = function() {
  var rf_this=this; // use obj inside other functions

  var base_el=document.getElementById(this.html_id);

  this.res_id=this.html_id+"_res";
  this.ds_cb_id=this.html_id+"_ds_cb";
  this.graph_id=this.html_id+"_graph";
  this.scale_id=this.html_id+"_scale";
  this.legend_sel_id=this.html_id+"_legend_sel";
  this.time_sel_id=this.html_id+"_time_sel";
  this.elem_group_id=this.html_id+"_elem_group";

  // First clean up anything in the element
  while (base_el.lastChild!=null) base_el.removeChild(base_el.lastChild);

  // Now create the layout
  var external_table=document.createElement("Table");
  this.external_table=external_table;

  // Header two: resulution select and DS selection title
  var rowHeader=external_table.insertRow(-1);
  var cellRes=rowHeader.insertCell(-1);
  cellRes.colSpan=3;
  cellRes.appendChild(document.createTextNode("Resolution:"));
  var forRes=document.createElement("Select");
  forRes.id=this.res_id;
  //forRes.onChange= this.callback_res_changed;
  forRes.onchange= function () {rf_this.callback_res_changed();};
  cellRes.appendChild(forRes);
  
  var cellDSTitle=rowHeader.insertCell(-1);
  cellDSTitle.appendChild(document.createTextNode("Select elements to plot:"));

  // Graph row: main graph and DS selection block
  var rowGraph=external_table.insertRow(-1);
  var cellGraph=rowGraph.insertCell(-1);
  cellGraph.colSpan=3;
  var elGraph=document.createElement("Div");
  if(this.rrdflot_defaults.graph_width!=null) {
     elGraph.style.width=this.rrdflot_defaults.graph_width;
  } else {elGraph.style.width="500px";}
  if(this.rrdflot_defaults.graph_height!=null) {
     elGraph.style.height=this.rrdflot_defaults.graph_height;
  } else {elGraph.style.height="300px";}
  elGraph.id=this.graph_id;
  cellGraph.appendChild(elGraph);

  var cellDScb=rowGraph.insertCell(-1);
  

  cellDScb.vAlign="top";
  var formDScb=document.createElement("Form");
  formDScb.id=this.ds_cb_id;
  formDScb.onchange= function () {rf_this.callback_ds_cb_changed();};
  cellDScb.appendChild(formDScb);

  // Scale row: scaled down selection graph
  var rowScale=external_table.insertRow(-1);

  var cellScaleLegend=rowScale.insertCell(-1);
  cellScaleLegend.vAlign="top";
  cellScaleLegend.appendChild(document.createTextNode("Legend:"));
  cellScaleLegend.appendChild(document.createElement('br'));

  var forScaleLegend=document.createElement("Select");
  forScaleLegend.id=this.legend_sel_id;
  forScaleLegend.appendChild(new Option("Top","nw",this.rrdflot_defaults.legend=="Top",this.rrdflot_defaults.legend=="Top"));
  forScaleLegend.appendChild(new Option("Bottom","sw",this.rrdflot_defaults.legend=="Bottom",this.rrdflot_defaults.legend=="Bottom"));
  forScaleLegend.appendChild(new Option("TopRight","ne",this.rrdflot_defaults.legend=="TopRight",this.rrdflot_defaults.legend=="TopRight"));
  forScaleLegend.appendChild(new Option("BottomRight","se",this.rrdflot_defaults.legend=="BottomRight",this.rrdflot_defaults.legend=="BottomRight"));
  forScaleLegend.appendChild(new Option("None","None",this.rrdflot_defaults.legend=="None",this.rrdflot_defaults.legend=="None"));
  forScaleLegend.onchange= function () {rf_this.callback_legend_changed();};
  cellScaleLegend.appendChild(forScaleLegend);


  cellScaleLegend.appendChild(document.createElement('br'));
  cellScaleLegend.appendChild(document.createTextNode("Timezone:"));
  cellScaleLegend.appendChild(document.createElement('br'));

  var timezone=document.createElement("Select");
  timezone.id=this.time_sel_id;

  var timezones = ["+12","+11","+10","+9","+8","+7","+6","+5","+4","+3","+2","+1","0",
                  "-1","-2","-3","-4","-5","-6","-7","-8","-9","-10","-11","-12"];
  var tz_found=false;
  var true_tz;
  for(var j=0; j<24; j++) {
    if (Math.ceil(this.rrdflot_defaults.timezone)==Math.ceil(timezones[j])) {
      tz_found=true;
      true_tz=Math.ceil(this.rrdflot_defaults.timezone);
      break;
    }
  }
  if (!tz_found) {
    // the passed timezone does not make sense
    // find the local time
    var d= new Date();
    true_tz=-Math.ceil(d.getTimezoneOffset()/60);
  }
  for(var j=0; j<24; j++) {
    timezone.appendChild(new Option(timezones[j],timezones[j],true_tz==Math.ceil(timezones[j]),true_tz==Math.ceil(timezones[j])));
  }
  timezone.onchange= function () {rf_this.callback_timezone_changed();};

  cellScaleLegend.appendChild(timezone);

  var cellScale=rowScale.insertCell(-1);
  cellScale.align="right";
  var elScale=document.createElement("Div");
  if(this.rrdflot_defaults.scale_width!=null) {
     elScale.style.width=this.rrdflot_defaults.scale_width;
  } else {elScale.style.width="250px";}
  if(this.rrdflot_defaults.scale_height!=null) {
     elScale.style.height=this.rrdflot_defaults.scale_height;
  } else {elScale.style.height="110px";}
  elScale.id=this.scale_id;
  cellScale.appendChild(elScale);
  
  var cellScaleReset=rowScale.insertCell(-1);
  cellScaleReset.vAlign="top";
  cellScaleReset.appendChild(document.createTextNode(" "));
  cellScaleReset.appendChild(document.createElement('br'));
  var elScaleReset=document.createElement("input");
  elScaleReset.type = "button";
  elScaleReset.value = "Reset selection";
  elScaleReset.onclick = function () {rf_this.callback_scale_reset();}

  cellScaleReset.appendChild(elScaleReset);

  base_el.appendChild(external_table);
};

// ===============================================
// Remove all HTMl elements but the graph
rrdFlot.prototype.cleanHTMLCruft = function() {
  var rf_this=this; // use obj inside other functions

  // delete top and bottom rows... graph is in the middle
  this.external_table.deleteRow(-1);
  this.external_table.deleteRow(0);

  var ds_el=document.getElementById(this.ds_cb_id);
  ds_el.removeChild(ds_el.lastChild);
}

// ======================================
// Populate RRA and RD info
rrdFlot.prototype.populateRes = function() {
  var form_el=document.getElementById(this.res_id);

  // First clean up anything in the element
  while (form_el.lastChild!=null) form_el.removeChild(form_el.lastChild);

  // now populate with RRA info
  var nrRRAs=this.rrd_file.getNrRRAs();
  for (var i=0; i<nrRRAs; i++) {

    var rra=this.rrd_file.getRRAInfo(i);
    var step=rra.getStep();
    var rows=rra.getNrRows();
    var period=step*rows;
    var rra_label=rfs_format_time(step)+" ("+rfs_format_time(period)+" total)";
    if (this.rrdflot_defaults.multi_rra) rra_label+=" "+rra.getCFName();
    form_el.appendChild(new Option(rra_label,i));
  }
    if(this.rrdflot_defaults.use_rra) {form_el.selectedIndex = this.rrdflot_defaults.rra;}
};

rrdFlot.prototype.populateDScb = function() {
  var rf_this=this; // use obj inside other functions
  var form_el=document.getElementById(this.ds_cb_id);
 
  //Create a table within a table to arrange
  // checkbuttons into two or more columns
  var table_el=document.createElement("Table");
  var row_el=table_el.insertRow(-1);
  row_el.vAlign="top";
  var cell_el=null; // will define later

  if (this.rrdflot_defaults.num_cb_rows==null) {
     this.rrdflot_defaults.num_cb_rows=12; 
  }
  // now populate with DS info
  var nrDSs=this.rrd_file.getNrDSs();
  var elem_group_number = 0;
 
  for (var i=0; i<nrDSs; i++) {

    if ((i%this.rrdflot_defaults.num_cb_rows)==0) { // one column every x DSs
      if(this.rrdflot_defaults.use_element_buttons) {
        cell_el=row_el.insertCell(-1); //make next element column 
        if(nrDSs>this.rrdflot_defaults.num_cb_rows) { //if only one column, no need for a button
          elem_group_number = (i/this.rrdflot_defaults.num_cb_rows)+1;
          var elGroupSelect = document.createElement("input");
          elGroupSelect.type = "button";
          elGroupSelect.value = "Group "+elem_group_number;
          elGroupSelect.onclick = (function(e) { //lambda function!!
             return function() {rf_this.callback_elem_group_changed(e);};})(elem_group_number);

          cell_el.appendChild(elGroupSelect);
          cell_el.appendChild(document.createElement('br')); //add space between the two
        }
      } else {
         //just make next element column
         cell_el=row_el.insertCell(-1); 
      }
    }
    var ds=this.rrd_file.getDS(i);
    if (this.rrdflot_defaults.multi_ds) { //null==false in boolean ops
       var name=ds.getName()+"-"+ds.getType();
       var name2=ds.getName();
    }
    else {var name=ds.getName(); var name2=ds.getName();}
    var title=name; 
    if(this.rrdflot_defaults.use_checked_DSs) {
       if(this.rrdflot_defaults.checked_DSs.length==0) {
          var checked=(i==0); // only first checked by default
       } else{checked=false;}
    } else {var checked=(i==0);}
    if (this.ds_graph_options[name]!=null) {
      var dgo=this.ds_graph_options[name];
      if (dgo['title']!=null) {
	// if the user provided the title, use it
	title=dgo['title'];
      } else if (dgo['label']!=null) {
	// use label as a second choiceit
	title=dgo['label'];
      } // else leave the ds name
      if(this.rrdflot_defaults.use_checked_DSs) {
         if(this.rrdflot_defaults.checked_DSs.length==0) {
           // if the user provided the title, use it
           checked=dgo['checked'];
         }
      } else {
         if (dgo['checked']!=null) {
            checked=dgo['checked']; 
         }
      }
    }
    if(this.rrdflot_defaults.use_checked_DSs) {
       if(this.rrdflot_defaults.checked_DSs==null) {continue;}
       for(var j=0;j<this.rrdflot_defaults.checked_DSs.length;j++){
             if (name==this.rrdflot_defaults.checked_DSs[j]) {checked=true;}
       }
    }
    var cb_el = document.createElement("input");
    cb_el.type = "checkbox";
    cb_el.name = "ds";
    cb_el.value = name2;
    cb_el.checked = cb_el.defaultChecked = checked;
    cell_el.appendChild(cb_el);
    cell_el.appendChild(document.createTextNode(title));
    cell_el.appendChild(document.createElement('br'));
  }
  form_el.appendChild(table_el);
};

// ======================================
// 
rrdFlot.prototype.drawFlotGraph = function() {
  // Res contains the RRA idx
  var oSelect=document.getElementById(this.res_id);
  var rra_idx=Number(oSelect.options[oSelect.selectedIndex].value);
  selected_rra=rra_idx;
  if(this.rrdflot_defaults.use_rra) {
    oSelect.options[oSelect.selectedIndex].value = this.rrdflot_defaults.rra;
    rra_idx = this.rrdflot_defaults.rra;
  }

  // now get the list of selected DSs
  var ds_positive_stack_list=[];
  var ds_negative_stack_list=[];
  var ds_single_list=[];
  var ds_colors={};
  var oCB=document.getElementById(this.ds_cb_id);
  var nrDSs=oCB.ds.length;
  local_checked_DSs=[];
  if (oCB.ds.length>0) {
    for (var i=0; i<oCB.ds.length; i++) {
      if (oCB.ds[i].checked==true) {
	var ds_name=oCB.ds[i].value;
	var ds_stack_type='none';
        local_checked_DSs.push(ds_name);;
	if (this.ds_graph_options[ds_name]!=null) {
	  var dgo=this.ds_graph_options[ds_name];
	  if (dgo['stack']!=null) {
	    var ds_stack_type=dgo['stack'];
	  }
	}
	if (ds_stack_type=='positive') {
	  ds_positive_stack_list.push(ds_name);
	} else if (ds_stack_type=='negative') {
	  ds_negative_stack_list.push(ds_name);
	} else {
	  ds_single_list.push(ds_name);
	}
	ds_colors[ds_name]=i;
      }
    }
  } else { // single element is not treated as an array
    if (oCB.ds.checked==true) {
      // no sense trying to stack a single element
      var ds_name=oCB.ds.value;
      ds_single_list.push(ds_name);
      ds_colors[ds_name]=0;
      local_checked_DSs.push(ds_name);
    }
  }

  var timeSelect=document.getElementById(this.time_sel_id);
  timezone_shift=timeSelect.options[timeSelect.selectedIndex].value;

  // then extract RRA data about those DSs
  var flot_obj=rrdRRAStackFlotObj(this.rrd_file,rra_idx,
				  ds_positive_stack_list,ds_negative_stack_list,ds_single_list,
                                  timezone_shift*3600);

  // fix the colors, based on the position in the RRD
  for (var i=0; i<flot_obj.data.length; i++) {
    var name=flot_obj.data[i].label; // at this point, label is the ds_name
    var color=ds_colors[name]; // default color as defined above
    if (this.ds_graph_options[name]!=null) {
      var dgo=this.ds_graph_options[name];
      if (dgo['color']!=null) {
	color=dgo['color'];
      }
      if (dgo['label']!=null) {
	// if the user provided the label, use it
	flot_obj.data[i].label=dgo['label'];
      } else  if (dgo['title']!=null) {
	// use title as a second choice 
	flot_obj.data[i].label=dgo['title'];
      } // else use the ds name
      if (dgo['lines']!=null) {
	// if the user provided the label, use it
	flot_obj.data[i].lines=dgo['lines'];
      }
      if (dgo['yaxis']!=null) {
	// if the user provided the label, use it
	flot_obj.data[i].yaxis=dgo['yaxis'];
      }
    }
    flot_obj.data[i].color=color;
  }

  // finally do the real plotting
  this.bindFlotGraph(flot_obj);
};

// ======================================
// Bind the graphs to the HTML tags
rrdFlot.prototype.bindFlotGraph = function(flot_obj) {
  var rf_this=this; // use obj inside other functions

  // Legend
  var oSelect=document.getElementById(this.legend_sel_id);
  var legend_id=oSelect.options[oSelect.selectedIndex].value;
  var graph_jq_id="#"+this.graph_id;
  var scale_jq_id="#"+this.scale_id;

  var graph_options = {
    legend: {show:false, position:"nw",noColumns:3},
    lines: {show:true},
    xaxis: { mode: "time" },
    yaxis: { autoscaleMargin: 0.20},
    selection: { mode: "x" },
    tooltip: true,
    tooltipOpts: { content: "<h4>%s</h4> Value: %y.3" },
    grid: { hoverable: true },
  };
  
  if (legend_id=="None") {
    // do nothing
  } else {
    graph_options.legend.show=true;
    graph_options.legend.position=legend_id;
  }

  if (this.graph_options!=null) {
    graph_options=populateGraphOptions(graph_options,this.graph_options);
  }

  if (graph_options.tooltip==false) {
    // avoid the need for the caller specify both
    graph_options.grid.hoverable=false;
  }

  if (this.selection_range.isSet()) {
    var selection_range=this.selection_range.getFlotRanges();
    if(this.rrdflot_defaults.use_windows) {
       graph_options.xaxis.min = this.rrdflot_defaults.window_min;  
       graph_options.xaxis.max = this.rrdflot_defaults.window_max;  
    } else {
    graph_options.xaxis.min=selection_range.xaxis.from;
    graph_options.xaxis.max=selection_range.xaxis.to;
    }
  } else if(this.rrdflot_defaults.use_windows) {
    graph_options.xaxis.min = this.rrdflot_defaults.window_min;  
    graph_options.xaxis.max = this.rrdflot_defaults.window_max;  
  } else {
    graph_options.xaxis.min=flot_obj.min;
    graph_options.xaxis.max=flot_obj.max;
  }

  var scale_options = {
    legend: {show:false},
    lines: {show:true},
    xaxis: {mode: "time", min:flot_obj.min, max:flot_obj.max },
    yaxis: graph_options.yaxis,
    selection: { mode: "x" },
  };

  //this.selection_range.selection_min=flot_obj.min;
  //this.selection_range.selection_max=flot_obj.max;

  var flot_data=flot_obj.data;
  var graph_data=this.selection_range.trim_flot_data(flot_data);
  var scale_data=flot_data;

  this.graph = $.plot($(graph_jq_id), graph_data, graph_options);
  this.scale = $.plot($(scale_jq_id), scale_data, scale_options);
 
  
  if(this.rrdflot_defaults.use_windows) {
    ranges = {};
    ranges.xaxis = [];
    ranges.xaxis.from = this.rrdflot_defaults.window_min;
    ranges.xaxis.to = this.rrdflot_defaults.window_max;
    rf_this.scale.setSelection(ranges,true);
    window_min = ranges.xaxis.from;
    window_max = ranges.xaxis.to;
  }

  if (this.selection_range.isSet()) {
    this.scale.setSelection(this.selection_range.getFlotRanges(),true); //don't fire event, no need
  }

  // now connect the two    
  $(graph_jq_id).unbind("plotselected"); // but first remove old function
  $(graph_jq_id).bind("plotselected", function (event, ranges) {
      // do the zooming
      rf_this.selection_range.setFromFlotRanges(ranges);
      graph_options.xaxis.min=ranges.xaxis.from;
      graph_options.xaxis.max=ranges.xaxis.to;
      window_min = ranges.xaxis.from;
      window_max = ranges.xaxis.to;
      rf_this.graph = $.plot($(graph_jq_id), rf_this.selection_range.trim_flot_data(flot_data), graph_options);
      
      // don't fire event on the scale to prevent eternal loop
      rf_this.scale.setSelection(ranges, true); //puts the transparent window on minigraph
  });
   
  $(scale_jq_id).unbind("plotselected"); //same here 
  $(scale_jq_id).bind("plotselected", function (event, ranges) {
      rf_this.graph.setSelection(ranges);
  });

  // only the scale has a selection
  // so when that is cleared, redraw also the graph
  $(scale_jq_id).bind("plotunselected", function() {
      rf_this.selection_range.reset();
      graph_options.xaxis.min=flot_obj.min;
      graph_options.xaxis.max=flot_obj.max;
      rf_this.graph = $.plot($(graph_jq_id), rf_this.selection_range.trim_flot_data(flot_data), graph_options);
      window_min = 0;
      window_max = 0;
  });
};

// callback functions that are called when one of the selections changes
rrdFlot.prototype.callback_res_changed = function() {
  this.rrdflot_defaults.use_rra = false;
  this.drawFlotGraph();
};

rrdFlot.prototype.callback_ds_cb_changed = function() {
  this.drawFlotGraph();
};

rrdFlot.prototype.callback_scale_reset = function() {
  this.scale.clearSelection();
};

rrdFlot.prototype.callback_legend_changed = function() {
  this.drawFlotGraph();
};

rrdFlot.prototype.callback_timezone_changed = function() {
  this.drawFlotGraph();
};

rrdFlot.prototype.callback_elem_group_changed = function(num) { //,window_min,window_max) {

  var oCB=document.getElementById(this.ds_cb_id);
  var nrDSs=oCB.ds.length;
  if (oCB.ds.length>0) {
    for (var i=0; i<oCB.ds.length; i++) {
      if(Math.floor(i/this.rrdflot_defaults.num_cb_rows)==num-1) {oCB.ds[i].checked=true; }
      else {oCB.ds[i].checked=false;}
    }
  }
  this.drawFlotGraph()
};

function getGraphInfo() {
   var graph_info = {};
   graph_info['dss'] = local_checked_DSs;
   graph_info['rra'] = selected_rra;
   graph_info['window_min'] = window_min;
   graph_info['window_max'] = window_max;
   graph_info['timezone'] = timezone_shift;
   return graph_info;
};

function resetWindow() {
  window_min = 0;
  window_max = 0; 
};

function populateGraphOptions(me, other) {
  for (e in other) {
    if (me[e]!=undefined) {
      if (Object.prototype.toString.call(other[e])=="[object Object]") {
	me[e]=populateGraphOptions(me[e],other[e]);
      } else {
	me[e]=other[e];
      }
    } else {
      /// create a new one
      if (Object.prototype.toString.call(other[e])=="[object Object]") {
	// This will do a deep copy
	me[e]=populateGraphOptions({},other[e]);
      } else {
	me[e]=other[e];
      }
    }
  }
  return me;
};
/*
 * RRD graphing libraries, based on Flot
 * Part of the javascriptRRD package
 * Copyright (c) 2010 Frank Wuerthwein, fkw@ucsd.edu
 *                    Igor Sfiligoi, isfiligoi@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

/*
 *
 * Flot is a javascript plotting library developed and maintained by
 * Ole Laursen [http://www.flotcharts.org/]
 *
 */

/*
 * The rrd_files is a list of 
 *  [rrd_id,rrd_file] pairs
 * All rrd_files must have the same step, the same DSes and the same number of RRAs.
 *
 */ 

/*
 * The ds_list is a list of 
 *  [ds_id, ds_title] pairs
 * If not defined, the list will be created from the RRDs
 *
 */ 

/*
 * Local dependencies:
 *  rrdFlotSupport.py
 *
 * External dependencies:
 *  [Flot]/jquery.py
 *  [Flot]/jquery.flot.js
 *  [Flot]/jquery.flot.selection.js
 */

/* graph_options defaults (see Flot docs for details)
 * {
 *  legend: { position:"nw",noColumns:3},
 *  lines: { show:true },
 *  yaxis: { autoscaleMargin: 0.20}
 * }
 *
 * rrd_graph_options is a dictionary of rrd_id, 
 *   with each element being a graph_option
 *   The defaults for each element are
 *   {
 *     title: label  or rrd_name                          // this is what is displayed in the checkboxes
 *     checked: true                                      // boolean
 *     label: title or rrd_name                           // this is what is displayed in the legend
 *     color: rrd_index                                   // see Flot docs for details
 *     lines: { show:true, fill: true, fillColor:color }  // see Flot docs for details
 *   }
 *
 * //overwrites other defaults; mostly used for linking via the URL
 * rrdflot_defaults defaults (see Flot docs for details) 	 
 * {
 *    graph_only: false        // If true, limit the display to the graph only
 *    legend: "Top"            //Starting location of legend. Options are: 
 *                             //   "Top","Bottom","TopRight","BottomRight","None".
 *    num_cb_rows: 12          //How many rows of DS checkboxes per column.
 *    use_element_buttons: false  //To be used in conjunction with num_cb_rows: This option
 *                             //    creates a button above every column, which selects
 *                             //    every element in the column. 
 *    multi_rra: false         //"true" appends the name of the RRA consolidation function (CF) 
 *                             //    (AVERAGE, MIN, MAX or LAST) to the name of the RRA. Useful 
 *                             //    for RRAs over the same interval with different CFs.  
 *    use_checked_RRDs: false   //Use the list checked_RRDs below.
 *    checked_RRDs: []          //List of elements to be checked by default when graph is loaded. 
 *                             //    Overwrites graph options. 
 *    use_rra: false           //Whether to use the rra index specified below.
 *    rra: 0                   //RRA (rra index in rrd) to be selected when graph is loaded. 
 *    use_windows: false       //Whether to use the window zoom specifications below.
 *    window_min: 0            //Sets minimum for window zoom. X-axis usually in unix time. 
 *    window_max: 0            //Sets maximum for window zoom.
 *    graph_height: "300px"    //Height of main graph. 
 *    graph_width: "500px"     //Width of main graph.
 *    scale_height: "110px"    //Height of small scaler graph.
 *    scale_width: "250px"     //Width of small scaler graph.
 * } 
 */

var local_checked_RRDs = [];
var selected_rra = 0;
var window_min=0;
var window_max=0;
var elem_group=null;


function rrdFlotMatrix(html_id, rrd_files, ds_list, graph_options, rrd_graph_options,rrdflot_defaults) {
  this.html_id=html_id;
  this.rrd_files=rrd_files;
  if (rrdflot_defaults==null) {
    this.rrdflot_defaults=new Object(); // empty object, just not to be null
  } else {
    this.rrdflot_defaults=rrdflot_defaults;
  }
  if (ds_list==null) {
    this.ds_list=[];
    var rrd_file=this.rrd_files[0][1]; // get the first one... they are all the same
    var nrDSs=rrd_file.getNrDSs();
    for (var i=0; i<nrDSs; i++) {
      var ds=this.rrd_files[0][1].getDS(i);
      var name=ds.getName();
      this.ds_list.push([name,name]);
    }
  } else {
    this.ds_list=ds_list;
  }
  this.graph_options=graph_options;
  if (rrd_graph_options==null) {
    this.rrd_graph_options=new Object(); // empty object, just not to be null
  } else {
    this.rrd_graph_options=rrd_graph_options;
  }
  this.selection_range=new rrdFlotSelection();

  this.createHTML();
  this.populateDS();
  this.populateRes();
  this.populateRRDcb();
  this.drawFlotGraph()

  if (this.rrdflot_defaults.graph_only==true) {
    this.cleanHTMLCruft();
  }
}


// ===============================================
// Create the HTML tags needed to host the graphs
rrdFlotMatrix.prototype.createHTML = function() {
  var rf_this=this; // use obj inside other functions

  var base_el=document.getElementById(this.html_id);

  this.ds_id=this.html_id+"_ds";
  this.res_id=this.html_id+"_res";
  this.rrd_cb_id=this.html_id+"_rrd_cb";
  this.graph_id=this.html_id+"_graph";
  this.scale_id=this.html_id+"_scale";
  this.legend_sel_id=this.html_id+"_legend_sel";

  // First clean up anything in the element
  while (base_el.lastChild!=null) base_el.removeChild(base_el.lastChild);

  // Now create the layout
  var external_table=document.createElement("Table");
  this.external_table=external_table;

  // DS rows: select DS
  var rowDS=external_table.insertRow(-1);
  var cellDS=rowDS.insertCell(-1);
  cellDS.colSpan=4
  cellDS.appendChild(document.createTextNode("Element:"));
  var forDS=document.createElement("Select");
  forDS.id=this.ds_id;
  forDS.onchange= function () {rf_this.callback_ds_changed();};
  cellDS.appendChild(forDS);

  // Header row: resulution select and DS selection title
  var rowHeader=external_table.insertRow(-1);
  var cellRes=rowHeader.insertCell(-1);
  cellRes.colSpan=3;
  cellRes.appendChild(document.createTextNode("Resolution:"));
  var forRes=document.createElement("Select");
  forRes.id=this.res_id;
  forRes.onchange= function () {rf_this.callback_res_changed();};
  cellRes.appendChild(forRes);

  var cellRRDTitle=rowHeader.insertCell(-1);
  cellRRDTitle.appendChild(document.createTextNode("Select RRDs to plot:"));

  // Graph row: main graph and DS selection block
  var rowGraph=external_table.insertRow(-1);
  var cellGraph=rowGraph.insertCell(-1);
  cellGraph.colSpan=3;
  var elGraph=document.createElement("Div");
  if(this.rrdflot_defaults.graph_width!=null) {
     elGraph.style.width=this.rrdflot_defaults.graph_width;
  } else {elGraph.style.width="500px";}
  if(this.rrdflot_defaults.graph_height!=null) {
     elGraph.style.height=this.rrdflot_defaults.graph_height;
  } else {elGraph.style.height="300px";}
  elGraph.id=this.graph_id;
  cellGraph.appendChild(elGraph);

  var cellRRDcb=rowGraph.insertCell(-1);
  cellRRDcb.vAlign="top";
  var formRRDcb=document.createElement("Form");
  formRRDcb.id=this.rrd_cb_id;
  formRRDcb.onchange= function () {rf_this.callback_rrd_cb_changed();};
  cellRRDcb.appendChild(formRRDcb);

  // Scale row: scaled down selection graph
  var rowScale=external_table.insertRow(-1);

  var cellScaleLegend=rowScale.insertCell(-1);
  cellScaleLegend.vAlign="top";
  cellScaleLegend.appendChild(document.createTextNode("Legend:"));
  cellScaleLegend.appendChild(document.createElement('br'));
  var forScaleLegend=document.createElement("Select");
  forScaleLegend.id=this.legend_sel_id;
  forScaleLegend.appendChild(new Option("Top","nw",this.rrdflot_defaults.legend=="Top",this.rrdflot_defaults.legend=="Top"));
  forScaleLegend.appendChild(new Option("Bottom","sw",this.rrdflot_defaults.legend=="Bottom",this.rrdflot_defaults.legend=="Bottom"));
  forScaleLegend.appendChild(new Option("TopRight","ne",this.rrdflot_defaults.legend=="TopRight",this.rrdflot_defaults.legend=="TopRight"));
  forScaleLegend.appendChild(new Option("BottomRight","se",this.rrdflot_defaults.legend=="BottomRight",this.rrdflot_defaults.legend=="BottomRight"));
  forScaleLegend.appendChild(new Option("None","None",this.rrdflot_defaults.legend=="None",this.rrdflot_defaults.legend=="None"));
  forScaleLegend.onchange= function () {rf_this.callback_legend_changed();};
  cellScaleLegend.appendChild(forScaleLegend);

  var cellScale=rowScale.insertCell(-1);
  cellScale.align="right";
  var elScale=document.createElement("Div");
  if(this.rrdflot_defaults.scale_width!=null) {
     elScale.style.width=this.rrdflot_defaults.scale_width;
  } else {elScale.style.width="250px";}
  if(this.rrdflot_defaults.scale_height!=null) {
     elScale.style.height=this.rrdflot_defaults.scale_height;
  } else {elScale.style.height="110px";}
  elScale.id=this.scale_id;
  cellScale.appendChild(elScale);
  
  var cellScaleReset=rowScale.insertCell(-1);
  cellScaleReset.vAlign="top";
  cellScaleReset.appendChild(document.createTextNode(" "));
  cellScaleReset.appendChild(document.createElement('br'));
  var elScaleReset=document.createElement("input");
  elScaleReset.type = "button";
  elScaleReset.value = "Reset selection";
  elScaleReset.onclick = function () {rf_this.callback_scale_reset();}
  cellScaleReset.appendChild(elScaleReset);


  base_el.appendChild(external_table);
};

// ===============================================
// Remove all HTMl elements but the graph
rrdFlotMatrix.prototype.cleanHTMLCruft = function() {
  var rf_this=this; // use obj inside other functions

  // delete 2 top and 1 bottom rows... graph is in the middle
  this.external_table.deleteRow(-1);
  this.external_table.deleteRow(0);
  this.external_table.deleteRow(0);

  var rrd_el=document.getElementById(this.rrd_cb_id);
  rrd_el.removeChild(rrd_el.lastChild);
}

// ======================================
// Populate DSs, RRA and RRD info
rrdFlotMatrix.prototype.populateDS = function() {
  var form_el=document.getElementById(this.ds_id);

  // First clean up anything in the element
  while (form_el.lastChild!=null) form_el.removeChild(form_el.lastChild);

  for (i in this.ds_list) {
    var ds=this.ds_list[i];
    form_el.appendChild(new Option(ds[1],ds[0]));
  }
};

rrdFlotMatrix.prototype.populateRes = function() {
  var form_el=document.getElementById(this.res_id);

  // First clean up anything in the element
  while (form_el.lastChild!=null) form_el.removeChild(form_el.lastChild);

  var rrd_file=this.rrd_files[0][1]; // get the first one... they are all the same
  // now populate with RRA info
  var nrRRAs=rrd_file.getNrRRAs();
  for (var i=0; i<nrRRAs; i++) {
    var rra=rrd_file.getRRAInfo(i);
    var step=rra.getStep();
    var rows=rra.getNrRows();
    var period=step*rows;
    var rra_label=rfs_format_time(step)+" ("+rfs_format_time(period)+" total)";
    if (this.rrdflot_defaults.multi_rra) rra_label+=" "+rra.getCFName();
    form_el.appendChild(new Option(rra_label,i));
  }
  if(this.rrdflot_defaults.use_rra) {form_el.selectedIndex = this.rrdflot_defaults.rra;}
};

rrdFlotMatrix.prototype.populateRRDcb = function() {
  var rf_this=this; // use obj inside other functions
  var form_el=document.getElementById(this.rrd_cb_id);
 
  //Create a table within a table to arrange
  // checkbuttons into two or more columns
  var table_el=document.createElement("Table");
  var row_el=table_el.insertRow(-1);
  row_el.vAlign="top";
  var cell_el=null; // will define later

  if (this.rrdflot_defaults.num_cb_rows==null) {
     this.rrdflot_defaults.num_cb_rows=12; 
  }
  // now populate with RRD info
  var nrRRDs=this.rrd_files.length;
  var elem_group_number = 0;
 
  for (var i=0; i<nrRRDs; i++) {

    if ((i%this.rrdflot_defaults.num_cb_rows)==0) { // one column every x RRDs
      if(this.rrdflot_defaults.use_element_buttons) {
        cell_el=row_el.insertCell(-1); //make next element column 
        if(nrRRDs>this.rrdflot_defaults.num_cb_rows) { //if only one column, no need for a button
          elem_group_number = (i/this.rrdflot_defaults.num_cb_rows)+1;
          var elGroupSelect = document.createElement("input");
          elGroupSelect.type = "button";
          elGroupSelect.value = "Group "+elem_group_number;
          elGroupSelect.onclick = (function(e) { //lambda function!!
             return function() {rf_this.callback_elem_group_changed(e);};})(elem_group_number);

          cell_el.appendChild(elGroupSelect);
          cell_el.appendChild(document.createElement('br')); //add space between the two
        }
      } else {
         //just make next element column
         cell_el=row_el.insertCell(-1); 
      }
    }

    var rrd_el=this.rrd_files[i];
    var rrd_file=rrd_el[1];
    var name=rrd_el[0];
    var title=name;
 
    if(this.rrdflot_defaults.use_checked_RRDs) {
       if(this.rrdflot_defaults.checked_RRDs.length==0) {
          var checked=(i==0); // only first checked by default
       } else{checked=false;}
    } else {var checked=(i==0);}
    if (this.rrd_graph_options[name]!=null) {
      var rgo=this.rrd_graph_options[name];
      if (rgo['title']!=null) {
	// if the user provided the title, use it
	title=rgo['title'];
      } else if (rgo['label']!=null) {
	// use label as a second choiceit
	title=rgo['label'];
      } // else leave the rrd name
      if(this.rrdflot_defaults.use_checked_RRDs) {
         if(this.rrdflot_defaults.checked_RRDs.length==0) {
           // if the user provided the title, use it
           checked=rgo['checked'];
         }
      } else {
         if (rgo['checked']!=null) {
            checked=rgo['checked']; 
         }
      }
    }
    if(this.rrdflot_defaults.use_checked_RRDs) {
       if(this.rrdflot_defaults.checked_RRDs==null) {continue;}
       for(var j=0;j<this.rrdflot_defaults.checked_RRDs.length;j++){
             if (name==this.rrdflot_defaults.checked_RRDs[j]) {checked=true;}
       }
    }
    var cb_el = document.createElement("input");
    cb_el.type = "checkbox";
    cb_el.name = "rrd";
    cb_el.value = i;
    cb_el.checked = cb_el.defaultChecked = checked;
    cell_el.appendChild(cb_el);
    cell_el.appendChild(document.createTextNode(title));
    cell_el.appendChild(document.createElement('br'));
  }
  form_el.appendChild(table_el);
};

// ======================================
// 
rrdFlotMatrix.prototype.drawFlotGraph = function() {
  // DS
  var oSelect=document.getElementById(this.ds_id);
  var ds_id=oSelect.options[oSelect.selectedIndex].value;

  // Res contains the RRA idx
  oSelect=document.getElementById(this.res_id);
  var rra_idx=Number(oSelect.options[oSelect.selectedIndex].value);

  selected_rra=rra_idx;
  if(this.rrdflot_defaults.use_rra) {
    oSelect.options[oSelect.selectedIndex].value = this.rrdflot_defaults.rra;
    rra_idx = this.rrdflot_defaults.rra;
  }

  // Extract ds info ... to be finished
  var ds_positive_stack=null;

  var std_colors=["#00ff00","#00ffff","#0000ff","#ff00ff",
		  "#808080","#ff0000","#ffff00","#e66266",
		  "#33cccc","#fff8a9","#ccffff","#a57e81",
		  "#7bea81","#8d4dff","#ffcc99","#000000"];

  // now get the list of selected RRDs
  var rrd_list=[];
  var rrd_colors=[];
  var oCB=document.getElementById(this.rrd_cb_id);
  var nrRRDs=oCB.rrd.length;
  if (oCB.rrd.length>0) {
    for (var i=0; i<oCB.rrd.length; i++) {
      if (oCB.rrd[i].checked==true) {
	//var rrd_idx=Number(oCB.rrd[i].value);
	rrd_list.push(this.rrd_files[i]);
	color=std_colors[i%std_colors.length];
	if ((i/std_colors.length)>=1) {
	  // wraparound, change them a little
	  idiv=Math.floor(i/std_colors.length);
	  c1=parseInt(color[1]+color[2],16);
	  c2=parseInt(color[3]+color[4],16);
	  c3=parseInt(color[5]+color[6],16);
	  m1=Math.floor((c1-128)/Math.sqrt(idiv+1))+128;
	  m2=Math.floor((c2-128)/Math.sqrt(idiv+1))+128;
	  m3=Math.floor((c3-128)/Math.sqrt(idiv+1))+128;
	  if (m1>15) s1=(m1).toString(16); else s1="0"+(m1).toString(16);
	  if (m2>15) s2=(m2).toString(16); else s2="0"+(m2).toString(16);
	  if (m3>15) s3=(m3).toString(16); else s3="0"+(m3).toString(16);
	  color="#"+s1+s2+s3;
	}
        rrd_colors.push(color);
      }
    }
  } else { // single element is not treated as an array
    if (oCB.rrd.checked==true) {
      // no sense trying to stack a single element
      rrd_list.push(this.rrd_files[0]);
      rrd_colors.push(std_colors[0]);
    }
  }
  
  // then extract RRA data about those DSs... to be finished
  var flot_obj=rrdRRAMultiStackFlotObj(rrd_list,rra_idx,ds_id);

  // fix the colors, based on the position in the RRD
  for (var i=0; i<flot_obj.data.length; i++) {
    var name=flot_obj.data[i].label; // at this point, label is the rrd_name
    var color=rrd_colors[flot_obj.data.length-i-1]; // stack inverts colors
    var lines=null;
    if (this.rrd_graph_options[name]!=null) {
      var dgo=this.rrd_graph_options[name];
      if (dgo['color']!=null) {
	color=dgo['color'];
      }
      if (dgo['label']!=null) {
	// if the user provided the label, use it
	flot_obj.data[i].label=dgo['label'];
      } else  if (dgo['title']!=null) {
	// use title as a second choice 
	flot_obj.data[i].label=dgo['title'];
      } // else use the rrd name
      if (dgo['lines']!=null) {
	// if the user provided the label, use it
	flot_obj.data[i].lines=dgo['lines'];
      }
    }
    if (lines==null) {
	flot_obj.data[i].lines= { show:true, fill: true, fillColor:color };
    }
    flot_obj.data[i].color=color;
  }

  // finally do the real plotting
  this.bindFlotGraph(flot_obj);
};

// ======================================
// Bind the graphs to the HTML tags
rrdFlotMatrix.prototype.bindFlotGraph = function(flot_obj) {
  var rf_this=this; // use obj inside other functions

  // Legend
  var oSelect=document.getElementById(this.legend_sel_id);
  var legend_id=oSelect.options[oSelect.selectedIndex].value;
  var graph_jq_id="#"+this.graph_id;
  var scale_jq_id="#"+this.scale_id;

  var graph_options = {
    legend: {show:false, position:"nw",noColumns:3},
    lines: {show:true},
    xaxis: { mode: "time" },
    yaxis: { autoscaleMargin: 0.20},
    selection: { mode: "x" },
    tooltip: true,
    tooltipOpts: { content: "<h4>%s</h4> Value: %y.3" },
    grid: { hoverable: true },
  };
  
  if (legend_id=="None") {
    // do nothing
  } else {
    graph_options.legend.show=true;
    graph_options.legend.position=legend_id;
  }

  if (this.graph_options!=null) {
    graph_options=populateGraphOptions(graph_options,this.graph_options);
  }

  if (graph_options.tooltip==false) {
    // avoid the need for the caller specify both
    graph_options.grid.hoverable=false;
  }


  if (this.selection_range.isSet()) {
    var selection_range=this.selection_range.getFlotRanges();
    if(this.rrdflot_defaults.use_windows) {
       graph_options.xaxis.min = this.rrdflot_defaults.window_min;  
       graph_options.xaxis.max = this.rrdflot_defaults.window_max;  
    } else {
    graph_options.xaxis.min=selection_range.xaxis.from;
    graph_options.xaxis.max=selection_range.xaxis.to;
    }
  } else if(this.rrdflot_defaults.use_windows) {
    graph_options.xaxis.min = this.rrdflot_defaults.window_min;  
    graph_options.xaxis.max = this.rrdflot_defaults.window_max;  
  } else {
    graph_options.xaxis.min=flot_obj.min;
    graph_options.xaxis.max=flot_obj.max;
  }

  var scale_options = {
    legend: {show:false},
    lines: {show:true},
    xaxis: { mode: "time", min:flot_obj.min, max:flot_obj.max },
    selection: { mode: "x" },
  };
    
  var flot_data=flot_obj.data;

  var graph_data=this.selection_range.trim_flot_data(flot_data);
  var scale_data=flot_data;

  this.graph = $.plot($(graph_jq_id), graph_data, graph_options);
  this.scale = $.plot($(scale_jq_id), scale_data, scale_options);

  if(this.rrdflot_defaults.use_windows) {
    ranges = {};
    ranges.xaxis = [];
    ranges.xaxis.from = this.rrdflot_defaults.window_min;
    ranges.xaxis.to = this.rrdflot_defaults.window_max;
    rf_this.scale.setSelection(ranges,true);
    window_min = ranges.xaxis.from;
    window_max = ranges.xaxis.to;
  }

  if (this.selection_range.isSet()) {
    this.scale.setSelection(this.selection_range.getFlotRanges(),true); //don't fire event, no need
  }

  // now connect the two    
  $(graph_jq_id).bind("plotselected", function (event, ranges) {
      // do the zooming
      rf_this.selection_range.setFromFlotRanges(ranges);
      graph_options.xaxis.min=ranges.xaxis.from;
      graph_options.xaxis.max=ranges.xaxis.to;
      window_min = ranges.xaxis.from;
      window_max = ranges.xaxis.to;
      rf_this.graph = $.plot($(graph_jq_id), rf_this.selection_range.trim_flot_data(flot_data), graph_options);
      
      // don't fire event on the scale to prevent eternal loop
      rf_this.scale.setSelection(ranges, true);
  });
    
  $(scale_jq_id).bind("plotselected", function (event, ranges) {
      rf_this.graph.setSelection(ranges);
  });

  // only the scale has a selection
  // so when that is cleared, redraw also the graph
  $(scale_jq_id).bind("plotunselected", function() {
      rf_this.selection_range.reset();
      graph_options.xaxis.min=flot_obj.min;
      graph_options.xaxis.max=flot_obj.max;
      rf_this.graph = $.plot($(graph_jq_id), rf_this.selection_range.trim_flot_data(flot_data), graph_options);
  });
};

// callback functions that are called when one of the selections changes
rrdFlotMatrix.prototype.callback_res_changed = function() {
  this.drawFlotGraph();
};

rrdFlotMatrix.prototype.callback_ds_changed = function() {
  this.drawFlotGraph();
};

rrdFlotMatrix.prototype.callback_rrd_cb_changed = function() {
  this.drawFlotGraph();
};

rrdFlotMatrix.prototype.callback_scale_reset = function() {
  this.scale.clearSelection();
};

rrdFlotMatrix.prototype.callback_legend_changed = function() {
  this.drawFlotGraph();
};

rrdFlotMatrix.prototype.callback_elem_group_changed = function(num) { 

  var oCB=document.getElementById(this.rrd_cb_id);
  var nrRRDs=oCB.rrd.length;
  if (oCB.rrd.length>0) {
    for (var i=0; i<oCB.rrd.length; i++) {
      if(Math.floor(i/this.rrdflot_defaults.num_cb_rows)==num-1) {oCB.rrd[i].checked=true; }
      else {oCB.rrd[i].checked=false;}
    }
  }
  this.drawFlotGraph()
};

function populateGraphOptions(me, other) {
  for (e in other) {
    if (me[e]!=undefined) {
      if (Object.prototype.toString.call(other[e])=="[object Object]") {
	me[e]=populateGraphOptions(me[e],other[e]);
      } else {
	me[e]=other[e];
      }
    } else {
      /// create a new one
      if (Object.prototype.toString.call(other[e])=="[object Object]") {
	// This will do a deep copy
	me[e]=populateGraphOptions({},other[e]);
      } else {
	me[e]=other[e];
      }
    }
  }
  return me;
};
/*
 * Filter classes for rrdFile
 * They implement the same interface, but changing the content
 * 
 * Part of the javascriptRRD package
 * Copyright (c) 2009 Frank Wuerthwein, fkw@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

/*
 * All filter classes must implement the following interface:
 *     getMinStep()
 *     getLastUpdate()
 *     getNrRRAs()
 *     getRRAInfo(rra_idx)
 *     getFilterRRA(rra_idx)
 *     getName()
 *
 * Where getFilterRRA returns an object implementing the following interface:
 *     getIdx()
 *     getNrRows()
 *     getStep()
 *     getCFName()
 *     getEl(row_idx)
 *     getElFast(row_idx)
 *
 */


// ================================================================
// Filter out a subset of DSs (identified either by idx or by name)

// Internal
function RRDRRAFilterDS(rrd_rra,ds_list) {
  this.rrd_rra=rrd_rra;
  this.ds_list=ds_list;
}
RRDRRAFilterDS.prototype.getIdx = function() {return this.rrd_rra.getIdx();}
RRDRRAFilterDS.prototype.getNrRows = function() {return this.rrd_rra.getNrRows();}
RRDRRAFilterDS.prototype.getNrDSs = function() {return this.ds_list.length;}
RRDRRAFilterDS.prototype.getStep = function() {return this.rrd_rra.getStep();}
RRDRRAFilterDS.prototype.getCFName = function() {return this.rrd_rra.getCFName();}
RRDRRAFilterDS.prototype.getEl = function(row_idx,ds_idx) {
  if ((ds_idx>=0) && (ds_idx<this.ds_list.length)) {
    var real_ds_idx=this.ds_list[ds_idx].real_ds_idx;
    return this.rrd_rra.getEl(row_idx,real_ds_idx);
  } else {
    throw RangeError("DS idx ("+ ds_idx +") out of range [0-" + this.ds_list.length +").");
  }	
}
RRDRRAFilterDS.prototype.getElFast = function(row_idx,ds_idx) {
  if ((ds_idx>=0) && (ds_idx<this.ds_list.length)) {
    var real_ds_idx=this.ds_list[ds_idx].real_ds_idx;
    return this.rrd_rra.getElFast(row_idx,real_ds_idx);
  } else {
    throw RangeError("DS idx ("+ ds_idx +") out of range [0-" + this.ds_list.length +").");
  }	
}

// --------------------------------------------------
// Public
// NOTE: This class is deprecated, use RRDFilterOp instead
function RRDFilterDS(rrd_file,ds_id_list) {
  this.rrd_file=rrd_file;
  this.ds_list=[];
  for (var i=0; i<ds_id_list.length; i++) {
    var org_ds=rrd_file.getDS(ds_id_list[i]);
    // must create a new copy, as the index has changed
    var new_ds=new RRDDS(org_ds.rrd_data,org_ds.rrd_data_idx,i);
    // then extend it to include the real RRD index
    new_ds.real_ds_idx=org_ds.my_idx;

    this.ds_list.push(new_ds);
  }
}
RRDFilterDS.prototype.getMinStep = function() {return this.rrd_file.getMinStep();}
RRDFilterDS.prototype.getLastUpdate = function() {return this.rrd_file.getLastUpdate();}

RRDFilterDS.prototype.getNrDSs = function() {return this.ds_list.length;}
RRDFilterDS.prototype.getDSNames = function() {
  var ds_names=[];
  for (var i=0; i<this.ds_list.length; i++) {
    ds_names.push(ds_list[i].getName());
  }
  return ds_names;
}
RRDFilterDS.prototype.getDS = function(id) {
  if (typeof id == "number") {
    return this.getDSbyIdx(id);
  } else {
    return this.getDSbyName(id);
  }
}

// INTERNAL: Do not call directly
RRDFilterDS.prototype.getDSbyIdx = function(idx) {
  if ((idx>=0) && (idx<this.ds_list.length)) {
    return this.ds_list[idx];
  } else {
    throw RangeError("DS idx ("+ idx +") out of range [0-" + this.ds_list.length +").");
  }	
}

// INTERNAL: Do not call directly
RRDFilterDS.prototype.getDSbyName = function(name) {
  for (var idx=0; idx<this.ds_list.length; idx++) {
    var ds=this.ds_list[idx];
    var ds_name=ds.getName()
    if (ds_name==name)
      return ds;
  }
  throw RangeError("DS name "+ name +" unknown.");
}

RRDFilterDS.prototype.getNrRRAs = function() {return this.rrd_file.getNrRRAs();}
RRDFilterDS.prototype.getRRAInfo = function(idx) {return this.rrd_file.getRRAInfo(idx);}
RRDFilterDS.prototype.getRRA = function(idx) {return new RRDRRAFilterDS(this.rrd_file.getRRA(idx),this.ds_list);}

// ================================================================
// Filter out by using a user provided filter object
// The object must implement the following interface
//   getName()               - Symbolic name give to this function
//   getDSName()             - list of DSs used in computing the result (names or indexes)
//   computeResult(val_list) - val_list contains the values of the requested DSs (in the same order) 

// If the element is a string or a number, it will just use that ds

// Example class that implements the interface:
//   function DoNothing(ds_name) { //Leaves the DS alone.
//     this.getName = function() {return ds_name;}
//     this.getDSNames = function() {return [ds_name];}
//     this.computeResult = function(val_list) {return val_list[0];}
//   }
//   function sumDS(ds1_name,ds2_name) { //Sums the two DSs.
//     this.getName = function() {return ds1_name+"+"+ds2_name;}
//     this.getDSNames = function() {return [ds1_name,ds2_name];}
//     this.computeResult = function(val_list) {return val_list[0]+val_list[1];}
//   }
//
// So to add a summed DS of your 1st and second DS: 
// var ds0_name = rrd_data.getDS(0).getName();
// var ds1_name = rrd_data.getDS(1).getName();
// rrd_data = new RRDFilterOp(rrd_data, [new DoNothing(ds0_name), 
//                DoNothing(ds1_name), sumDS(ds0_name, ds1_name]);
//
// You get the same resoult with
// rrd_data = new RRDFilterOp(rrd_data, [ds0_name,1,new sumDS(ds0_name, ds1_name)]);
////////////////////////////////////////////////////////////////////

// this implements the conceptual NoNothing above
function RRDFltOpIdent(ds_name) {
     this.getName = function() {return ds_name;}
     this.getDSNames = function() {return [ds_name];}
     this.computeResult = function(val_list) {return val_list[0];}
}

// similar to the above, but extracts the name from the index
// requires two parametes, since it it need context
function RRDFltOpIdentId(rrd_data,id) {
     this.ds_name=rrd_data.getDS(id).getName();
     this.getName = function() {return this.ds_name;}
     this.getDSNames = function() {return [this.ds_name];}
     this.computeResult = function(val_list) {return val_list[0];}
}

//Private
function RRDDSFilterOp(rrd_file,op_obj,my_idx) {
  this.rrd_file=rrd_file;
  this.op_obj=op_obj;
  this.my_idx=my_idx;
  var ds_names=op_obj.getDSNames();
  var ds_idx_list=[];
  for (var i=0; i<ds_names.length; i++) {
    ds_idx_list.push(rrd_file.getDS(ds_names[i]).getIdx());
  }
  this.ds_idx_list=ds_idx_list;
}
RRDDSFilterOp.prototype.getIdx = function() {return this.my_idx;}
RRDDSFilterOp.prototype.getName = function() {return this.op_obj.getName();}

RRDDSFilterOp.prototype.getType = function() {return "function";}
RRDDSFilterOp.prototype.getMin = function() {return undefined;}
RRDDSFilterOp.prototype.getMax = function() {return undefined;}

// These are new to RRDDSFilterOp
RRDDSFilterOp.prototype.getRealDSList = function() { return this.ds_idx_list;}
RRDDSFilterOp.prototype.computeResult = function(val_list) {return this.op_obj.computeResult(val_list);}

// ------ --------------------------------------------
//Private
function RRDRRAFilterOp(rrd_rra,ds_list) {
  this.rrd_rra=rrd_rra;
  this.ds_list=ds_list;
}
RRDRRAFilterOp.prototype.getIdx = function() {return this.rrd_rra.getIdx();}
RRDRRAFilterOp.prototype.getNrRows = function() {return this.rrd_rra.getNrRows();}
RRDRRAFilterOp.prototype.getNrDSs = function() {return this.ds_list.length;}
RRDRRAFilterOp.prototype.getStep = function() {return this.rrd_rra.getStep();}
RRDRRAFilterOp.prototype.getCFName = function() {return this.rrd_rra.getCFName();}
RRDRRAFilterOp.prototype.getEl = function(row_idx,ds_idx) {
  if ((ds_idx>=0) && (ds_idx<this.ds_list.length)) {
    var ds_idx_list=this.ds_list[ds_idx].getRealDSList();
    var val_list=[];
    for (var i=0; i<ds_idx_list.length; i++) {
      val_list.push(this.rrd_rra.getEl(row_idx,ds_idx_list[i]));
    }
    return this.ds_list[ds_idx].computeResult(val_list);
  } else {
    throw RangeError("DS idx ("+ ds_idx +") out of range [0-" + this.ds_list.length +").");
  }	
}
RRDRRAFilterOp.prototype.getElFast = function(row_idx,ds_idx) {
  if ((ds_idx>=0) && (ds_idx<this.ds_list.length)) {
    var ds_idx_list=this.ds_list[ds_idx].getRealDSList();
    var val_list=[];
    for (var i=0; i<ds_idx_list.length; i++) {
      val_list.push(this.rrd_rra.getEl(row_idx,ds_idx_list[i]));
    }
    return this.ds_list[ds_idx].computeResult(val_list);
  } else {
    throw RangeError("DS idx ("+ ds_idx +") out of range [0-" + this.ds_list.length +").");
  }	
}

// --------------------------------------------------
//Public
function RRDFilterOp(rrd_file,op_obj_list) {
  this.rrd_file=rrd_file;
  this.ds_list=[];
  for (i in op_obj_list) {
    var el=op_obj_list[i];
    var outel=null;
    if (typeof(el)=="string") {outel=new RRDFltOpIdent(el);}
    else if (typeof(el)=="number") {outel=new RRDFltOpIdentId(this.rrd_file,el);}
    else {outel=el;}
    this.ds_list.push(new RRDDSFilterOp(rrd_file,outel,i));
  }
}
RRDFilterOp.prototype.getMinStep = function() {return this.rrd_file.getMinStep();}
RRDFilterOp.prototype.getLastUpdate = function() {return this.rrd_file.getLastUpdate();}
RRDFilterOp.prototype.getNrDSs = function() {return this.ds_list.length;}
RRDFilterOp.prototype.getDSNames = function() {
  var ds_names=[];
  for (var i=0; i<this.ds_list.length; i++) {
    ds_names.push(ds_list[i].getName());
  }
  return ds_names;
}
RRDFilterOp.prototype.getDS = function(id) {
  if (typeof id == "number") {
    return this.getDSbyIdx(id);
  } else {
    return this.getDSbyName(id);
  }
}

// INTERNAL: Do not call directly
RRDFilterOp.prototype.getDSbyIdx = function(idx) {
  if ((idx>=0) && (idx<this.ds_list.length)) {
    return this.ds_list[idx];
  } else {
    throw RangeError("DS idx ("+ idx +") out of range [0-" + this.ds_list.length +").");
  }	
}

// INTERNAL: Do not call directly
RRDFilterOp.prototype.getDSbyName = function(name) {
  for (var idx=0; idx<this.ds_list.length; idx++) {
    var ds=this.ds_list[idx];
    var ds_name=ds.getName()
    if (ds_name==name)
      return ds;
  }
  throw RangeError("DS name "+ name +" unknown.");
}

RRDFilterOp.prototype.getNrRRAs = function() {return this.rrd_file.getNrRRAs();}
RRDFilterOp.prototype.getRRAInfo = function(idx) {return this.rrd_file.getRRAInfo(idx);}
RRDFilterOp.prototype.getRRA = function(idx) {return new RRDRRAFilterOp(this.rrd_file.getRRA(idx),this.ds_list);}

// ================================================================
// NOTE: This function is archaic, and will likely be deprecated in future releases
//
// Shift RRAs in rra_list by the integer shift_int (in seconds).
// Only change is getLastUpdate - this takes care of everything.
// Example: To shift the first three 3 RRAs in the file by one hour, 
//         rrd_data = new RRAFilterShift(rra_data, 3600, [0,1,2]);

function RRAFilterShift(rrd_file, shift_int, rra_list) {
  this.rrd_file = rrd_file;
  this.shift_int = shift_int;
  this.rra_list = rra_list;
  this.shift_in_seconds = this.shift_int*3600; //number of steps needed to move 1 hour
}
RRAFilterShift.prototype.getMinStep = function() {return this.rrd_file.getMinStep();}
RRAFilterShift.prototype.getLastUpdate = function() {return this.rrd_file.getLastUpdate()+this.shift_in_seconds;}
RRAFilterShift.prototype.getNrDSs = function() {return this.rrd_file.getNrDSs();}
RRAFilterShift.prototype.getDSNames = function() {return this.rrd_file.getDSNames();}
RRAFilterShift.prototype.getDS = function(id) {return this.rrd_file.getDS(id);}
RRAFilterShift.prototype.getNrRRAs = function() {return this.rra_list.length;}
RRAFilterShift.prototype.getRRAInfo = function(idx) {return this.rrd_file.getRRAInfo(idx);}
RRAFilterShift.prototype.getRRA = function(idx) {return this.rrd_file.getRRA(idx);}

// ================================================================
// Filter RRAs by using a user provided filter object
// The object must implement the following interface
//   getIdx()               - Index of RRA to use
//   getStep()              - new step size (return null to use step size of RRA specified by getIdx() 

// If a number is passed, it implies to just use the RRA as it is
// If an array is passed, it is assumed to be [rra_id,new_step_in_seconds] 
//    and a RRDRRAFltAvgOpNewStep object will be instantiated internally

/* Example classes that implements the interface:
*
*      //This RRA Filter object leaves the original RRA unchanged.
*
*      function RRADoNothing(rra_idx) {
*         this.getIdx = function() {return rra_idx;}
*         this.getStep = function() {return null;} 
*      }
*      
*      // This Filter creates a new RRA with a different step size 
*      // based on another RRA, whose data the new RRA averages. 
*      // rra_idx should be index of RRA with largest step size 
*      // that doesn't exceed new step size.  
*
*      function RRA_Avg(rra_idx,new_step_in_seconds) {
*         this.getIdx = function() {return rra_idx;}
*         this.getStep = function() {return new_step_in_seconds;}
*      }
*      //For example, if you have two RRAs, one with a 5 second step,
*      //and another with a 60 second step, and you'd like a 30 second step,
*      //rrd_data = new RRDRRAFilterAvg(rrd_data,[new RRADoNothing(0), new RRDDoNothing(1),new RRA_Avg(1,30)];)
*/

// Users can use this one directly for simple use cases
// It is equivalent to RRADoNothing and RRA_Avg above
function RRDRRAFltAvgOpNewStep(rra_idx,new_step_in_seconds) {
  this.getIdx = function() {return rra_idx;}
  this.getStep = function() {return new_step_in_seconds;}
}


//Private Function
function RRAInfoFilterAvg(rrd_file, rra, op_obj, idx) {
  this.rrd_file = rrd_file;
  this.op_obj = op_obj;
  this.base_rra = rrd_file.getRRA(this.op_obj.getIdx());
  this.rra = rra;
  this.idx = idx;
  var scaler = 1;
  if (this.op_obj.getStep()!=null) {scaler = this.op_obj.getStep()/this.base_rra.getStep();}
  this.scaler = scaler;
}
RRAInfoFilterAvg.prototype.getIdx = function() {return this.idx;}
RRAInfoFilterAvg.prototype.getNrRows = function() {return this.rra.getNrRows();} //draw info from RRAFilterAvg
RRAInfoFilterAvg.prototype.getStep = function() {return this.rra.getStep();}
RRAInfoFilterAvg.prototype.getCFName = function() {return this.rra.getCFName();}
RRAInfoFilterAvg.prototype.getPdpPerRow = function() {return this.rrd_file.getRRAInfo(this.op_obj.getIdx()).getPdpPerRow()*this.scaler;}

//---------------------------------------------------------------------------
//Private Function
function RRAFilterAvg(rrd_file, op_obj) {
  this.rrd_file = rrd_file;
  this.op_obj = op_obj;
  this.base_rra = rrd_file.getRRA(op_obj.getIdx());
  var scaler=1; 
  if (op_obj.getStep()!=null) {scaler = op_obj.getStep()/this.base_rra.getStep();}
  this.scaler = Math.floor(scaler);
  //document.write(this.scaler+",");
}
RRAFilterAvg.prototype.getIdx = function() {return this.op_obj.getIdx();}
RRAFilterAvg.prototype.getCFName = function() {return this.base_rra.getCFName();}
RRAFilterAvg.prototype.getNrRows = function() {return Math.floor(this.base_rra.getNrRows()/this.scaler);}
RRAFilterAvg.prototype.getNrDSs = function() {return this.base_rra.getNrDSs();}
RRAFilterAvg.prototype.getStep = function() {
   if(this.op_obj.getStep()!=null) {
      return this.op_obj.getStep(); 
   } else { return this.base_rra.getStep();}
}
RRAFilterAvg.prototype.getEl = function(row,ds) {
   var sum=0;
   for(var i=0;i<this.scaler;i++) {
      sum += this.base_rra.getEl((this.scaler*row)+i,ds);
   }
   return sum/this.scaler;
}
RRAFilterAvg.prototype.getElFast = function(row,ds) {
   var sum=0;
   for(var i=0;i<this.scaler;i++) {
      sum += this.base_rra.getElFast((this.scaler*row)+i,ds);
   }
   return sum/this.scaler;
}

//----------------------------------------------------------------------------
//Public function - use this one for RRA averaging
function RRDRRAFilterAvg(rrd_file, op_obj_list) {
  this.rrd_file = rrd_file;
  this.op_obj_list = new Array();
  this.rra_list=[];
  for (var i in op_obj_list) {
    var el=op_obj_list[i];
    var outel=null;
    if (Object.prototype.toString.call(el)=="[object Number]") {outel=new RRDRRAFltAvgOpNewStep(el,null);}
    else if (Object.prototype.toString.call(el)=="[object Array]") {outel=new RRDRRAFltAvgOpNewStep(el[0],el[1]);}
    else {outel=el;}
    this.op_obj_list.push(outel);
    this.rra_list.push(new RRAFilterAvg(rrd_file,outel));
  }
}
RRDRRAFilterAvg.prototype.getMinStep = function() {return this.rrd_file.getMinStep();} //other RRA steps change, not min
RRDRRAFilterAvg.prototype.getLastUpdate = function() {return this.rrd_file.getLastUpdate();}
RRDRRAFilterAvg.prototype.getNrDSs = function() {return this.rrd_file.getNrDSs();} //DSs unchanged
RRDRRAFilterAvg.prototype.getDSNames = function() {return this.rrd_file.getDSNames();}
RRDRRAFilterAvg.prototype.getDS = function(id) {return this.rrd_file.getDS(id);}
RRDRRAFilterAvg.prototype.getNrRRAs = function() {return this.rra_list.length;} 
RRDRRAFilterAvg.prototype.getRRAInfo = function(idx) {
  if ((idx>=0) && (idx<this.rra_list.length)) {
    return new RRAInfoFilterAvg(this.rrd_file, this.rra_list[idx],this.op_obj_list[idx],idx); 
  } else {return this.rrd_file.getRRAInfo(0);}
}
RRDRRAFilterAvg.prototype.getRRA = function(idx) {
  if ((idx>=0) && (idx<this.rra_list.length)) {
    return this.rra_list[idx];
  }
}


/*
 * Combine multiple rrdFiles into one object
 * It implements the same interface, but changing the content
 * 
 * Part of the javascriptRRD package
 * Copyright (c) 2010 Igor Sfiligoi, isfiligoi@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

// ============================================================
// RRD RRA handling class
function RRDRRASum(rra_list,offset_list,treat_undefined_as_zero) {
  this.rra_list=rra_list;
  this.offset_list=offset_list;
  this.treat_undefined_as_zero=treat_undefined_as_zero;
  this.row_cnt= this.rra_list[0].getNrRows();
}

RRDRRASum.prototype.getIdx = function() {
  return this.rra_list[0].getIdx();
}

// Get number of rows/columns
RRDRRASum.prototype.getNrRows = function() {
  return this.row_cnt;
}
RRDRRASum.prototype.getNrDSs = function() {
  return this.rra_list[0].getNrDSs();
}

// Get RRA step (expressed in seconds)
RRDRRASum.prototype.getStep = function() {
  return this.rra_list[0].getStep();
}

// Get consolidation function name
RRDRRASum.prototype.getCFName = function() {
  return this.rra_list[0].getCFName();
}

RRDRRASum.prototype.getEl = function(row_idx,ds_idx) {
  var outSum=0.0;
  for (var i in this.rra_list) {
    var offset=this.offset_list[i];
    if ((row_idx+offset)<this.row_cnt) {
      var rra=this.rra_list[i];
      val=rra.getEl(row_idx+offset,ds_idx);
    } else {
      /* out of row range -> undefined*/
      val=undefined;
    }
    /* treat all undefines as 0 for now */
    if (val==undefined) {
      if (this.treat_undefined_as_zero) {
	val=0;
      } else {
	/* if even one element is undefined, the whole sum is undefined */
	outSum=undefined;
	break;
      }
    }
    outSum+=val;
  }
  return outSum;
}

// Low precision version of getEl
// Uses getFastDoubleAt
RRDRRASum.prototype.getElFast = function(row_idx,ds_idx) {
  var outSum=0.0;
  for (var i in this.rra_list) {
    var offset=this.offset_list[i];
    if ((row_id+offset)<this.row_cnt) {
      var rra=this.rra_list[i];
      val=rra.getElFast(row_idx+offset,ds_idx);
    } else {
      /* out of row range -> undefined*/
      val=undefined;
    }
    /* treat all undefines as 0 for now */
    if (val==undefined) {
      if (this.treat_undefined_as_zero) {
	val=0;
      } else {
	/* if even one element is undefined, the whole sum is undefined */
	outSum=undefined;
	break;
      }
    }
    outSum+=val;
  }
  return outSum;
}

/*** INTERNAL ** sort by lastupdate, descending ***/

function rrdFileSort(f1, f2) {
  return f2.getLastUpdate()-f1.getLastUpdate();
}

/*
 * Sum several RRDfiles together
 * They must all have the same DSes and the same RRAs
 */ 


/*
 * sumfile_options, if defined, must be an object containing any of these
 *   treat_undefined_as_zero
 *
 */

// For backwards comatibility, if sumfile_options is a boolean,
// it is interpreted like the "old treat_undefined_as_zero" argument

function RRDFileSum(file_list,sumfile_options) {
  if (sumfile_options==undefined) {
    sumfile_options={};
  } else if (typeof(sumfile_options)=="boolean") {
    sumfile_options={treat_undefined_as_zero:sumfile_options};
  }
  this.sumfile_options=sumfile_options;

  
  if (this.sumfile_options.treat_undefined_as_zero==undefined) {
    this.treat_undefined_as_zero=true;
   } else {
    this.treat_undefined_as_zero=this.sumfile_options.treat_undefined_as_zero;
  }
  this.file_list=file_list;
  this.file_list.sort();

  // ===================================
  // Start of user functions

  this.getMinStep = function() {
    return this.file_list[0].getMinStep();
  }
  this.getLastUpdate = function() {
    return this.file_list[0].getLastUpdate();
  }

  this.getNrDSs = function() {
    return this.file_list[0].getNrDSs();
  }

  this.getDSNames = function() {
    return this.file_list[0].getDSNames();
  }

  this.getDS = function(id) {
    return this.file_list[0].getDS(id);
  }

  this.getNrRRAs = function() {
    return this.file_list[0].getNrRRAs();
  }

  this.getRRAInfo = function(idx) {
    return this.file_list[0].getRRAInfo(idx);
  }

  this.getRRA = function(idx) {
    var rra_info=this.getRRAInfo(idx);
    var rra_step=rra_info.getStep();
    var realLastUpdate=undefined;

    var rra_list=new Array();
    var offset_list=new Array();
    for (var i in this.file_list) {
      file=file_list[i];
      fileLastUpdate=file.getLastUpdate();
      if (realLastUpdate!=undefined) {
	fileSkrew=Math.floor((realLastUpdate-fileLastUpdate)/rra_step);
      } else {
	fileSkrew=0;
	firstLastUpdate=fileLastUpdate;
      }
      offset_list.push(fileSkrew);
      fileRRA=file.getRRA(idx);
      rra_list.push(fileRRA);
    }

    return new RRDRRASum(rra_list,offset_list,this.treat_undefined_as_zero);
  }

}
/*
 * Async versions of the rrdFlot class
 * Part of the javascriptRRD package
 * Copyright (c) 2013 Igor Sfiligoi, isfiligoi@ucsd.edu
 *
 * Original repository: http://javascriptrrd.sourceforge.net/
 * 
 * MIT License [http://www.opensource.org/licenses/mit-license.php]
 *
 */

/*
 * Local dependencies:
 *  binaryXHR.js
 *  rrdFile.js and/or rrdMultiFile.js
 *  optionally rrdFilter.js
 *  rrdFlot.js and/or rrdFlotMatrix.js
 *
 * Those modules may have other requirements.
 *
 */


/*
 * customization_callback = function(obj)
 *  This function, if defined, is called after the data has been loaded 
 *    and before the rrdFlot object is instantiated
 *
 *  The object passed as the sole argument is guaranteed to have the following arguments
 *    obj.rrd_data
 *    obj.graph_options
 *    obj.ds_graph_options or obj.rrd_graph_options
 *    obj.rrdflot_defaults
 *    obj.ds_op_list
 *    obj.rra_op_list
 *
 *  The purpose of this callback is to give the caller the option of personalizing
 *    the Flot graph based on the content of the rrd_data
 *    
/* Internal helper function */
function rrdFlotAsyncCallback(bf,obj) {
  var i_rrd_data=undefined;
  if (bf.getLength()<1) {
    alert("File "+obj.url+" is empty (possibly loading failed)!");
    return 1;
  }
  try {
    i_rrd_data=new RRDFile(bf,obj.file_options);
  } catch(err) {
    alert("File "+obj.url+" is not a valid RRD archive!\n"+err);
  }
  if (i_rrd_data!=undefined) {
    if (obj.rrd_data!=null) delete obj.rrd_data;
    obj.rrd_data=i_rrd_data;
    obj.callback();
  }
}

/* Use url==null if you do not know the url yet */
function rrdFlotAsync(html_id, url, 
		      file_options,                                      // see rrdFile.js::RRDFile for documentation
		      graph_options, ds_graph_options, rrdflot_defaults, // see rrdFlot.js::rrdFlot for documentation of these
		      ds_op_list,                                        // if defined, see rrdFilter.js::RRDFilterOp for documentation
		      rra_op_list,                                       // if defined, see rrdFilter.js::RRDRRAFilterAvg for documentation
                      customization_callback                             // if defined, see above
		      ) {
  this.html_id=html_id;
  this.url=url;
  this.file_options=file_options;
  this.graph_options=graph_options;
  this.ds_graph_options=ds_graph_options;
  this.rrdflot_defaults=rrdflot_defaults;
  this.ds_op_list=ds_op_list;
  this.rra_op_list=rra_op_list;

  this.customization_callback=customization_callback;

  this.rrd_flot_obj=null;
  this.rrd_data=null;

  if (url!=null) {
    this.reload(url);
  }
}

rrdFlotAsync.prototype.reload = function(url) {
  this.url=url;
  try {
    FetchBinaryURLAsync(url,rrdFlotAsyncCallback,this);
  } catch (err) {
    alert("Failed loading "+url+"\n"+err);
  }
};

rrdFlotAsync.prototype.callback = function() {
  if (this.rrd_flot_obj!=null) delete this.rrd_flot_obj;

  if (this.customization_callback!=undefined) this.customization_callback(this);

  var irrd_data=this.rrd_data;
  if (this.ds_op_list!=undefined) irrd_data=new RRDFilterOp(irrd_data,this.ds_op_list);
  if (this.rra_op_list!=undefined) irrd_data=new RRDRRAFilterAvg(irrd_data,this.rra_op_list);
  this.rrd_flot_obj=new rrdFlot(this.html_id, irrd_data, this.graph_options, this.ds_graph_options, this.rrdflot_defaults);
};


// ================================================================================================================

/* Internal helper function */
function rrdFlotMultiAsyncCallback(bf,arr) {
  var obj=arr[0];
  var idx=arr[1];

  obj.files_loaded++; // increase this even if it fails later on, else we will never finish

  var i_rrd_data=undefined;
  if (bf.getLength()<1) {
    alert("File "+obj.url_list[idx]+" is empty (possibly loading failed)! You may get a parial result in the graph.");
  } else {
    try {
      i_rrd_data=new RRDFile(bf,obj.file_options);
    } catch(err) {
      alert("File "+obj.url_list[idx]+" is not a valid RRD archive! You may get a partial result in the graph.\n"+err);
    }
  }
  if (i_rrd_data!=undefined) {
    obj.loaded_data[idx]=i_rrd_data;
  }

  if (obj.files_loaded==obj.files_needed) {
    obj.callback();
  }
}

/* Another internal helper function */
function rrdFlotMultiAsyncReload(obj,url_list) {
  obj.files_needed=url_list.length;
  obj.url_list=url_list;
  delete obj.loaded_data;
  obj.loaded_data=[];
  obj.files_loaded=0;
  for (i in url_list) {
    try {
      FetchBinaryURLAsync(url_list[i],rrdFlotMultiAsyncCallback,[obj,i]);
    } catch (err) {
      alert("Failed loading "+url_list[i]+". You may get a partial result in the graph.\n"+err);
      obj.files_needed--;
    }
  }
};



/* Use url_list==null if you do not know the urls yet */
function rrdFlotSumAsync(html_id, url_list,
			 file_options,                                      // see rrdFile.js::RRDFile for documentation
			 sumfile_options,                                   // see rrdMultiFile.js::RRDFileSum for documentation
			 graph_options, ds_graph_options, rrdflot_defaults, // see rrdFlot.js::rrdFlot for documentation of these
			 ds_op_list,                                        // if defined, see rrdFilter.js::RRDFilterOp for documentation
			 rra_op_list,                                       // if defined, see rrdFilter.js::RRDRRAFilterAvg for documentation
			 customization_callback                             // if defined, see above
		      ) {
  this.html_id=html_id;
  this.url_list=url_list;
  this.file_options=file_options;
  this.sumfile_options=sumfile_options;
  this.graph_options=graph_options;
  this.ds_graph_options=ds_graph_options;
  this.rrdflot_defaults=rrdflot_defaults;
  this.ds_op_list=ds_op_list;
  this.rra_op_list=rra_op_list;

  this.customization_callback=customization_callback;

  this.rrd_flot_obj=null;
  this.rrd_data=null; //rrd_data will contain the sum of all the loaded data

  this.loaded_data=null;

  if (url_list!=null) {
    this.reload(url_list);
  }
}

rrdFlotSumAsync.prototype.reload = function(url_list) {rrdFlotMultiAsyncReload(this,url_list);}


rrdFlotSumAsync.prototype.callback = function() {
  if (this.rrd_flot_obj!=null) delete this.rrd_flot_obj;
  var real_data_arr=new Array();
  for (i in this.loaded_data) {
    // account for failed loaded urls
    var el=this.loaded_data[i];
    if (el!=undefined) real_data_arr.push(el);
  }
  var rrd_sum=new RRDFileSum(real_data_arr,this.sumfile_options);
  if (this.rrd_data!=null) delete this.rrd_data;
  this.rrd_data=rrd_sum;

  if (this.customization_callback!=undefined) this.customization_callback(this);

  rrd_sum=this.rrd_data; // customization_callback may have altered it
  if (this.ds_op_list!=undefined) rrd_sum=new RRDFilterOp(rrd_sum,this.ds_op_list);
  if (this.rra_op_list!=undefined) rrd_sum=new RRDRRAFilterAvg(rrd_sum,this.rra_op_list);
  this.rrd_flot_obj=new rrdFlot(this.html_id, rrd_sum, this.graph_options, this.ds_graph_options, this.rrdflot_defaults);
};

// ================================================================================================================

/* Use url_list==null if you do not know the urls yet */
function rrdFlotMatrixAsync(html_id, 
			    url_pair_list,                                     // see rrdFlotMatrix.js::rrdFlotMatrix for documentation
			    file_options,                                      // see rrdFile.js::RRDFile for documentation
			    ds_list,                                           // see rrdFlotMatrix.js::rrdFlotMatrix for documentation
			    graph_options, rrd_graph_options, rrdflot_defaults, // see rrdFlotMatrix.js::rrdFlotMatrix for documentation of these
			    ds_op_list,                                        // if defined, see rrdFilter.js::RRDFilterOp for documentation
			    rra_op_list,                                       // if defined, see rrdFilter.js::RRDRRAFilterAvg for documentation
			    customization_callback                             // if defined, see above
			    ) {
  this.html_id=html_id;
  this.url_pair_list=url_pair_list;
  this.file_options=file_options;
  this.ds_list=ds_list;
  this.graph_options=graph_options;
  this.rrd_graph_options=rrd_graph_options;
  this.rrdflot_defaults=rrdflot_defaults;
  this.ds_op_list=ds_op_list;
  this.rra_op_list=rra_op_list;

  this.customization_callback=customization_callback;

  this.rrd_flot_obj=null;
  this.rrd_data=null; //rrd_data will contain the data of the first url; still useful to explore the DS and RRA structure

  this.loaded_data=null;

  this.url_list=null;
  if (url_pair_list!=null) {
    this.reload(url_pair_list);
  }
}

rrdFlotMatrixAsync.prototype.reload = function(url_pair_list) {
  this.url_pair_list=url_pair_list;
  var url_list=[];
  for (var i in this.url_pair_list) {
    url_list.push(this.url_pair_list[i][1]);
  }

  rrdFlotMultiAsyncReload(this,url_list);
}

rrdFlotMatrixAsync.prototype.callback = function() {
  if (this.rrd_flot_obj!=null) delete this.rrd_flot_obj;

  var real_data_arr=new Array();
  for (var i in this.loaded_data) {
    // account for failed loaded urls
    var el=this.loaded_data[i];
    if (el!=undefined) real_data_arr.push([this.url_pair_list[i][0],el]);
  }
  this.rrd_data=real_data_arr[0];

  if (this.customization_callback!=undefined) this.customization_callback(this);

  for (var i in real_data_arr) {
    if (this.ds_op_list!=undefined) real_data_arr[i]=new RRDFilterOp(real_data_arr[i],this.ds_op_list);
    if (this.rra_op_list!=undefined) real_data_arr[i]=new RRDRRAFilterAvg(real_data_arr[i],this.rra_op_list);
  }
  this.rrd_flot_obj=new rrdFlotMatrix(this.html_id, real_data_arr, this.ds_list, this.graph_options, this.rrd_graph_options, this.rrdflot_defaults);
};

/*
 * jQuery JavaScript Library v1.5.1
 * http://jquery.com/
 *
 * Copyright 2011, John Resig
 * Dual licensed under the MIT or GPL Version 2 licenses.
 * http://jquery.org/license
 *
 * Includes Sizzle.js
 * http://sizzlejs.com/
 * Copyright 2011, The Dojo Foundation
 * Released under the MIT, BSD, and GPL Licenses.
 *
 * Date: Wed Feb 23 13:55:29 2011 -0500
 */
(function(aY,H){var al=aY.document;var a=(function(){var bn=function(bI,bJ){return new bn.fn.init(bI,bJ,bl)},bD=aY.jQuery,bp=aY.$,bl,bH=/^(?:[^<]*(<[\w\W]+>)[^>]*$|#([\w\-]+)$)/,bv=/\S/,br=/^\s+/,bm=/\s+$/,bq=/\d/,bj=/^<(\w+)\s*\/?>(?:<\/\1>)?$/,bw=/^[\],:{}\s]*$/,bF=/\\(?:["\\\/bfnrt]|u[0-9a-fA-F]{4})/g,by=/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,bs=/(?:^|:|,)(?:\s*\[)+/g,bh=/(webkit)[ \/]([\w.]+)/,bA=/(opera)(?:.*version)?[ \/]([\w.]+)/,bz=/(msie) ([\w.]+)/,bB=/(mozilla)(?:.*? rv:([\w.]+))?/,bG=navigator.userAgent,bE,bC=false,bk,e="then done fail isResolved isRejected promise".split(" "),bd,bu=Object.prototype.toString,bo=Object.prototype.hasOwnProperty,bi=Array.prototype.push,bt=Array.prototype.slice,bx=String.prototype.trim,be=Array.prototype.indexOf,bg={};bn.fn=bn.prototype={constructor:bn,init:function(bI,bM,bL){var bK,bN,bJ,bO;if(!bI){return this}if(bI.nodeType){this.context=this[0]=bI;this.length=1;return this}if(bI==="body"&&!bM&&al.body){this.context=al;this[0]=al.body;this.selector="body";this.length=1;return this}if(typeof bI==="string"){bK=bH.exec(bI);if(bK&&(bK[1]||!bM)){if(bK[1]){bM=bM instanceof bn?bM[0]:bM;bO=(bM?bM.ownerDocument||bM:al);bJ=bj.exec(bI);if(bJ){if(bn.isPlainObject(bM)){bI=[al.createElement(bJ[1])];bn.fn.attr.call(bI,bM,true)}else{bI=[bO.createElement(bJ[1])]}}else{bJ=bn.buildFragment([bK[1]],[bO]);bI=(bJ.cacheable?bn.clone(bJ.fragment):bJ.fragment).childNodes}return bn.merge(this,bI)}else{bN=al.getElementById(bK[2]);if(bN&&bN.parentNode){if(bN.id!==bK[2]){return bL.find(bI)}this.length=1;this[0]=bN}this.context=al;this.selector=bI;return this}}else{if(!bM||bM.jquery){return(bM||bL).find(bI)}else{return this.constructor(bM).find(bI)}}}else{if(bn.isFunction(bI)){return bL.ready(bI)}}if(bI.selector!==H){this.selector=bI.selector;this.context=bI.context}return bn.makeArray(bI,this)},selector:"",jquery:"1.5.1",length:0,size:function(){return this.length},toArray:function(){return bt.call(this,0)},get:function(bI){return bI==null?this.toArray():(bI<0?this[this.length+bI]:this[bI])},pushStack:function(bJ,bL,bI){var bK=this.constructor();if(bn.isArray(bJ)){bi.apply(bK,bJ)}else{bn.merge(bK,bJ)}bK.prevObject=this;bK.context=this.context;if(bL==="find"){bK.selector=this.selector+(this.selector?" ":"")+bI}else{if(bL){bK.selector=this.selector+"."+bL+"("+bI+")"}}return bK},each:function(bJ,bI){return bn.each(this,bJ,bI)},ready:function(bI){bn.bindReady();bk.done(bI);return this},eq:function(bI){return bI===-1?this.slice(bI):this.slice(bI,+bI+1)},first:function(){return this.eq(0)},last:function(){return this.eq(-1)},slice:function(){return this.pushStack(bt.apply(this,arguments),"slice",bt.call(arguments).join(","))},map:function(bI){return this.pushStack(bn.map(this,function(bK,bJ){return bI.call(bK,bJ,bK)}))},end:function(){return this.prevObject||this.constructor(null)},push:bi,sort:[].sort,splice:[].splice};bn.fn.init.prototype=bn.fn;bn.extend=bn.fn.extend=function(){var bR,bK,bI,bJ,bO,bP,bN=arguments[0]||{},bM=1,bL=arguments.length,bQ=false;if(typeof bN==="boolean"){bQ=bN;bN=arguments[1]||{};bM=2}if(typeof bN!=="object"&&!bn.isFunction(bN)){bN={}}if(bL===bM){bN=this;--bM}for(;bM<bL;bM++){if((bR=arguments[bM])!=null){for(bK in bR){bI=bN[bK];bJ=bR[bK];if(bN===bJ){continue}if(bQ&&bJ&&(bn.isPlainObject(bJ)||(bO=bn.isArray(bJ)))){if(bO){bO=false;bP=bI&&bn.isArray(bI)?bI:[]}else{bP=bI&&bn.isPlainObject(bI)?bI:{}}bN[bK]=bn.extend(bQ,bP,bJ)}else{if(bJ!==H){bN[bK]=bJ}}}}}return bN};bn.extend({noConflict:function(bI){aY.$=bp;if(bI){aY.jQuery=bD}return bn},isReady:false,readyWait:1,ready:function(bI){if(bI===true){bn.readyWait--}if(!bn.readyWait||(bI!==true&&!bn.isReady)){if(!al.body){return setTimeout(bn.ready,1)}bn.isReady=true;if(bI!==true&&--bn.readyWait>0){return}bk.resolveWith(al,[bn]);if(bn.fn.trigger){bn(al).trigger("ready").unbind("ready")}}},bindReady:function(){if(bC){return}bC=true;if(al.readyState==="complete"){return setTimeout(bn.ready,1)}if(al.addEventListener){al.addEventListener("DOMContentLoaded",bd,false);aY.addEventListener("load",bn.ready,false)}else{if(al.attachEvent){al.attachEvent("onreadystatechange",bd);aY.attachEvent("onload",bn.ready);var bI=false;try{bI=aY.frameElement==null}catch(bJ){}if(al.documentElement.doScroll&&bI){bf()}}}},isFunction:function(bI){return bn.type(bI)==="function"},isArray:Array.isArray||function(bI){return bn.type(bI)==="array"},isWindow:function(bI){return bI&&typeof bI==="object"&&"setInterval" in bI},isNaN:function(bI){return bI==null||!bq.test(bI)||isNaN(bI)},type:function(bI){return bI==null?String(bI):bg[bu.call(bI)]||"object"},isPlainObject:function(bJ){if(!bJ||bn.type(bJ)!=="object"||bJ.nodeType||bn.isWindow(bJ)){return false}if(bJ.constructor&&!bo.call(bJ,"constructor")&&!bo.call(bJ.constructor.prototype,"isPrototypeOf")){return false}var bI;for(bI in bJ){}return bI===H||bo.call(bJ,bI)},isEmptyObject:function(bJ){for(var bI in bJ){return false}return true},error:function(bI){throw bI},parseJSON:function(bI){if(typeof bI!=="string"||!bI){return null}bI=bn.trim(bI);if(bw.test(bI.replace(bF,"@").replace(by,"]").replace(bs,""))){return aY.JSON&&aY.JSON.parse?aY.JSON.parse(bI):(new Function("return "+bI))()}else{bn.error("Invalid JSON: "+bI)}},parseXML:function(bK,bI,bJ){if(aY.DOMParser){bJ=new DOMParser();bI=bJ.parseFromString(bK,"text/xml")}else{bI=new ActiveXObject("Microsoft.XMLDOM");bI.async="false";bI.loadXML(bK)}bJ=bI.documentElement;if(!bJ||!bJ.nodeName||bJ.nodeName==="parsererror"){bn.error("Invalid XML: "+bK)}return bI},noop:function(){},globalEval:function(bK){if(bK&&bv.test(bK)){var bJ=al.head||al.getElementsByTagName("head")[0]||al.documentElement,bI=al.createElement("script");if(bn.support.scriptEval()){bI.appendChild(al.createTextNode(bK))}else{bI.text=bK}bJ.insertBefore(bI,bJ.firstChild);bJ.removeChild(bI)}},nodeName:function(bJ,bI){return bJ.nodeName&&bJ.nodeName.toUpperCase()===bI.toUpperCase()},each:function(bL,bP,bK){var bJ,bM=0,bN=bL.length,bI=bN===H||bn.isFunction(bL);if(bK){if(bI){for(bJ in bL){if(bP.apply(bL[bJ],bK)===false){break}}}else{for(;bM<bN;){if(bP.apply(bL[bM++],bK)===false){break}}}}else{if(bI){for(bJ in bL){if(bP.call(bL[bJ],bJ,bL[bJ])===false){break}}}else{for(var bO=bL[0];bM<bN&&bP.call(bO,bM,bO)!==false;bO=bL[++bM]){}}}return bL},trim:bx?function(bI){return bI==null?"":bx.call(bI)}:function(bI){return bI==null?"":bI.toString().replace(br,"").replace(bm,"")},makeArray:function(bL,bJ){var bI=bJ||[];if(bL!=null){var bK=bn.type(bL);if(bL.length==null||bK==="string"||bK==="function"||bK==="regexp"||bn.isWindow(bL)){bi.call(bI,bL)}else{bn.merge(bI,bL)}}return bI},inArray:function(bK,bL){if(bL.indexOf){return bL.indexOf(bK)}for(var bI=0,bJ=bL.length;bI<bJ;bI++){if(bL[bI]===bK){return bI}}return -1},merge:function(bM,bK){var bL=bM.length,bJ=0;if(typeof bK.length==="number"){for(var bI=bK.length;bJ<bI;bJ++){bM[bL++]=bK[bJ]}}else{while(bK[bJ]!==H){bM[bL++]=bK[bJ++]}}bM.length=bL;return bM},grep:function(bJ,bO,bI){var bK=[],bN;bI=!!bI;for(var bL=0,bM=bJ.length;bL<bM;bL++){bN=!!bO(bJ[bL],bL);if(bI!==bN){bK.push(bJ[bL])}}return bK},map:function(bJ,bO,bI){var bK=[],bN;for(var bL=0,bM=bJ.length;bL<bM;bL++){bN=bO(bJ[bL],bL,bI);if(bN!=null){bK[bK.length]=bN}}return bK.concat.apply([],bK)},guid:1,proxy:function(bK,bJ,bI){if(arguments.length===2){if(typeof bJ==="string"){bI=bK;bK=bI[bJ];bJ=H}else{if(bJ&&!bn.isFunction(bJ)){bI=bJ;bJ=H}}}if(!bJ&&bK){bJ=function(){return bK.apply(bI||this,arguments)}}if(bK){bJ.guid=bK.guid=bK.guid||bJ.guid||bn.guid++}return bJ},access:function(bI,bQ,bO,bK,bN,bP){var bJ=bI.length;if(typeof bQ==="object"){for(var bL in bQ){bn.access(bI,bL,bQ[bL],bK,bN,bO)}return bI}if(bO!==H){bK=!bP&&bK&&bn.isFunction(bO);for(var bM=0;bM<bJ;bM++){bN(bI[bM],bQ,bK?bO.call(bI[bM],bM,bN(bI[bM],bQ)):bO,bP)}return bI}return bJ?bN(bI[0],bQ):H},now:function(){return(new Date()).getTime()},_Deferred:function(){var bL=[],bM,bJ,bK,bI={done:function(){if(!bK){var bO=arguments,bP,bS,bR,bQ,bN;if(bM){bN=bM;bM=0}for(bP=0,bS=bO.length;bP<bS;bP++){bR=bO[bP];bQ=bn.type(bR);if(bQ==="array"){bI.done.apply(bI,bR)}else{if(bQ==="function"){bL.push(bR)}}}if(bN){bI.resolveWith(bN[0],bN[1])}}return this},resolveWith:function(bO,bN){if(!bK&&!bM&&!bJ){bJ=1;try{while(bL[0]){bL.shift().apply(bO,bN)}}catch(bP){throw bP}finally{bM=[bO,bN];bJ=0}}return this},resolve:function(){bI.resolveWith(bn.isFunction(this.promise)?this.promise():this,arguments);return this},isResolved:function(){return !!(bJ||bM)},cancel:function(){bK=1;bL=[];return this}};return bI},Deferred:function(bJ){var bI=bn._Deferred(),bL=bn._Deferred(),bK;bn.extend(bI,{then:function(bN,bM){bI.done(bN).fail(bM);return this},fail:bL.done,rejectWith:bL.resolveWith,reject:bL.resolve,isRejected:bL.isResolved,promise:function(bN){if(bN==null){if(bK){return bK}bK=bN={}}var bM=e.length;while(bM--){bN[e[bM]]=bI[e[bM]]}return bN}});bI.done(bL.cancel).fail(bI.cancel);delete bI.cancel;if(bJ){bJ.call(bI,bI)}return bI},when:function(bJ){var bO=arguments.length,bI=bO<=1&&bJ&&bn.isFunction(bJ.promise)?bJ:bn.Deferred(),bM=bI.promise();if(bO>1){var bN=bt.call(arguments,0),bL=bO,bK=function(bP){return function(bQ){bN[bP]=arguments.length>1?bt.call(arguments,0):bQ;if(!(--bL)){bI.resolveWith(bM,bN)}}};while((bO--)){bJ=bN[bO];if(bJ&&bn.isFunction(bJ.promise)){bJ.promise().then(bK(bO),bI.reject)}else{--bL}}if(!bL){bI.resolveWith(bM,bN)}}else{if(bI!==bJ){bI.resolve(bJ)}}return bM},uaMatch:function(bJ){bJ=bJ.toLowerCase();var bI=bh.exec(bJ)||bA.exec(bJ)||bz.exec(bJ)||bJ.indexOf("compatible")<0&&bB.exec(bJ)||[];return{browser:bI[1]||"",version:bI[2]||"0"}},sub:function(){function bJ(bL,bM){return new bJ.fn.init(bL,bM)}bn.extend(true,bJ,this);bJ.superclass=this;bJ.fn=bJ.prototype=this();bJ.fn.constructor=bJ;bJ.subclass=this.subclass;bJ.fn.init=function bK(bL,bM){if(bM&&bM instanceof bn&&!(bM instanceof bJ)){bM=bJ(bM)}return bn.fn.init.call(this,bL,bM,bI)};bJ.fn.init.prototype=bJ.fn;var bI=bJ(al);return bJ},browser:{}});bk=bn._Deferred();bn.each("Boolean Number String Function Array Date RegExp Object".split(" "),function(bJ,bI){bg["[object "+bI+"]"]=bI.toLowerCase()});bE=bn.uaMatch(bG);if(bE.browser){bn.browser[bE.browser]=true;bn.browser.version=bE.version}if(bn.browser.webkit){bn.browser.safari=true}if(be){bn.inArray=function(bI,bJ){return be.call(bJ,bI)}}if(bv.test("\xA0")){br=/^[\s\xA0]+/;bm=/[\s\xA0]+$/}bl=bn(al);if(al.addEventListener){bd=function(){al.removeEventListener("DOMContentLoaded",bd,false);bn.ready()}}else{if(al.attachEvent){bd=function(){if(al.readyState==="complete"){al.detachEvent("onreadystatechange",bd);bn.ready()}}}}function bf(){if(bn.isReady){return}try{al.documentElement.doScroll("left")}catch(bI){setTimeout(bf,1);return}bn.ready()}return bn})();(function(){a.support={};var bd=al.createElement("div");bd.style.display="none";bd.innerHTML="   <link/><table></table><a href='/a' style='color:red;float:left;opacity:.55;'>a</a><input type='checkbox'/>";var bm=bd.getElementsByTagName("*"),bk=bd.getElementsByTagName("a")[0],bl=al.createElement("select"),be=bl.appendChild(al.createElement("option")),bj=bd.getElementsByTagName("input")[0];if(!bm||!bm.length||!bk){return}a.support={leadingWhitespace:bd.firstChild.nodeType===3,tbody:!bd.getElementsByTagName("tbody").length,htmlSerialize:!!bd.getElementsByTagName("link").length,style:/red/.test(bk.getAttribute("style")),hrefNormalized:bk.getAttribute("href")==="/a",opacity:/^0.55$/.test(bk.style.opacity),cssFloat:!!bk.style.cssFloat,checkOn:bj.value==="on",optSelected:be.selected,deleteExpando:true,optDisabled:false,checkClone:false,noCloneEvent:true,noCloneChecked:true,boxModel:null,inlineBlockNeedsLayout:false,shrinkWrapBlocks:false,reliableHiddenOffsets:true};bj.checked=true;a.support.noCloneChecked=bj.cloneNode(true).checked;bl.disabled=true;a.support.optDisabled=!be.disabled;var bf=null;a.support.scriptEval=function(){if(bf===null){var bo=al.documentElement,bp=al.createElement("script"),br="script"+a.now();try{bp.appendChild(al.createTextNode("window."+br+"=1;"))}catch(bq){}bo.insertBefore(bp,bo.firstChild);if(aY[br]){bf=true;delete aY[br]}else{bf=false}bo.removeChild(bp);bo=bp=br=null}return bf};try{delete bd.test}catch(bh){a.support.deleteExpando=false}if(!bd.addEventListener&&bd.attachEvent&&bd.fireEvent){bd.attachEvent("onclick",function bn(){a.support.noCloneEvent=false;bd.detachEvent("onclick",bn)});bd.cloneNode(true).fireEvent("onclick")}bd=al.createElement("div");bd.innerHTML="<input type='radio' name='radiotest' checked='checked'/>";var bg=al.createDocumentFragment();bg.appendChild(bd.firstChild);a.support.checkClone=bg.cloneNode(true).cloneNode(true).lastChild.checked;a(function(){var bp=al.createElement("div"),e=al.getElementsByTagName("body")[0];if(!e){return}bp.style.width=bp.style.paddingLeft="1px";e.appendChild(bp);a.boxModel=a.support.boxModel=bp.offsetWidth===2;if("zoom" in bp.style){bp.style.display="inline";bp.style.zoom=1;a.support.inlineBlockNeedsLayout=bp.offsetWidth===2;bp.style.display="";bp.innerHTML="<div style='width:4px;'></div>";a.support.shrinkWrapBlocks=bp.offsetWidth!==2}bp.innerHTML="<table><tr><td style='padding:0;border:0;display:none'></td><td>t</td></tr></table>";var bo=bp.getElementsByTagName("td");a.support.reliableHiddenOffsets=bo[0].offsetHeight===0;bo[0].style.display="";bo[1].style.display="none";a.support.reliableHiddenOffsets=a.support.reliableHiddenOffsets&&bo[0].offsetHeight===0;bp.innerHTML="";e.removeChild(bp).style.display="none";bp=bo=null});var bi=function(e){var bp=al.createElement("div");e="on"+e;if(!bp.attachEvent){return true}var bo=(e in bp);if(!bo){bp.setAttribute(e,"return;");bo=typeof bp[e]==="function"}bp=null;return bo};a.support.submitBubbles=bi("submit");a.support.changeBubbles=bi("change");bd=bm=bk=null})();var aE=/^(?:\{.*\}|\[.*\])$/;a.extend({cache:{},uuid:0,expando:"jQuery"+(a.fn.jquery+Math.random()).replace(/\D/g,""),noData:{embed:true,object:"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000",applet:true},hasData:function(e){e=e.nodeType?a.cache[e[a.expando]]:e[a.expando];return !!e&&!P(e)},data:function(bf,bd,bh,bg){if(!a.acceptData(bf)){return}var bk=a.expando,bj=typeof bd==="string",bi,bl=bf.nodeType,e=bl?a.cache:bf,be=bl?bf[a.expando]:bf[a.expando]&&a.expando;if((!be||(bg&&be&&!e[be][bk]))&&bj&&bh===H){return}if(!be){if(bl){bf[a.expando]=be=++a.uuid}else{be=a.expando}}if(!e[be]){e[be]={};if(!bl){e[be].toJSON=a.noop}}if(typeof bd==="object"||typeof bd==="function"){if(bg){e[be][bk]=a.extend(e[be][bk],bd)}else{e[be]=a.extend(e[be],bd)}}bi=e[be];if(bg){if(!bi[bk]){bi[bk]={}}bi=bi[bk]}if(bh!==H){bi[bd]=bh}if(bd==="events"&&!bi[bd]){return bi[bk]&&bi[bk].events}return bj?bi[bd]:bi},removeData:function(bg,be,bh){if(!a.acceptData(bg)){return}var bj=a.expando,bk=bg.nodeType,bd=bk?a.cache:bg,bf=bk?bg[a.expando]:a.expando;if(!bd[bf]){return}if(be){var bi=bh?bd[bf][bj]:bd[bf];if(bi){delete bi[be];if(!P(bi)){return}}}if(bh){delete bd[bf][bj];if(!P(bd[bf])){return}}var e=bd[bf][bj];if(a.support.deleteExpando||bd!=aY){delete bd[bf]}else{bd[bf]=null}if(e){bd[bf]={};if(!bk){bd[bf].toJSON=a.noop}bd[bf][bj]=e}else{if(bk){if(a.support.deleteExpando){delete bg[a.expando]}else{if(bg.removeAttribute){bg.removeAttribute(a.expando)}else{bg[a.expando]=null}}}}},_data:function(bd,e,be){return a.data(bd,e,be,true)},acceptData:function(bd){if(bd.nodeName){var e=a.noData[bd.nodeName.toLowerCase()];if(e){return !(e===true||bd.getAttribute("classid")!==e)}}return true}});a.fn.extend({data:function(bg,bi){var bh=null;if(typeof bg==="undefined"){if(this.length){bh=a.data(this[0]);if(this[0].nodeType===1){var e=this[0].attributes,be;for(var bf=0,bd=e.length;bf<bd;bf++){be=e[bf].name;if(be.indexOf("data-")===0){be=be.substr(5);aT(this[0],be,bh[be])}}}}return bh}else{if(typeof bg==="object"){return this.each(function(){a.data(this,bg)})}}var bj=bg.split(".");bj[1]=bj[1]?"."+bj[1]:"";if(bi===H){bh=this.triggerHandler("getData"+bj[1]+"!",[bj[0]]);if(bh===H&&this.length){bh=a.data(this[0],bg);bh=aT(this[0],bg,bh)}return bh===H&&bj[1]?this.data(bj[0]):bh}else{return this.each(function(){var bl=a(this),bk=[bj[0],bi];bl.triggerHandler("setData"+bj[1]+"!",bk);a.data(this,bg,bi);bl.triggerHandler("changeData"+bj[1]+"!",bk)})}},removeData:function(e){return this.each(function(){a.removeData(this,e)})}});function aT(be,bd,bf){if(bf===H&&be.nodeType===1){bf=be.getAttribute("data-"+bd);if(typeof bf==="string"){try{bf=bf==="true"?true:bf==="false"?false:bf==="null"?null:!a.isNaN(bf)?parseFloat(bf):aE.test(bf)?a.parseJSON(bf):bf}catch(bg){}a.data(be,bd,bf)}else{bf=H}}return bf}function P(bd){for(var e in bd){if(e!=="toJSON"){return false}}return true}a.extend({queue:function(bd,e,bf){if(!bd){return}e=(e||"fx")+"queue";var be=a._data(bd,e);if(!bf){return be||[]}if(!be||a.isArray(bf)){be=a._data(bd,e,a.makeArray(bf))}else{be.push(bf)}return be},dequeue:function(bf,be){be=be||"fx";var e=a.queue(bf,be),bd=e.shift();if(bd==="inprogress"){bd=e.shift()}if(bd){if(be==="fx"){e.unshift("inprogress")}bd.call(bf,function(){a.dequeue(bf,be)})}if(!e.length){a.removeData(bf,be+"queue",true)}}});a.fn.extend({queue:function(e,bd){if(typeof e!=="string"){bd=e;e="fx"}if(bd===H){return a.queue(this[0],e)}return this.each(function(bf){var be=a.queue(this,e,bd);if(e==="fx"&&be[0]!=="inprogress"){a.dequeue(this,e)}})},dequeue:function(e){return this.each(function(){a.dequeue(this,e)})},delay:function(bd,e){bd=a.fx?a.fx.speeds[bd]||bd:bd;e=e||"fx";return this.queue(e,function(){var be=this;setTimeout(function(){a.dequeue(be,e)},bd)})},clearQueue:function(e){return this.queue(e||"fx",[])}});var aC=/[\n\t\r]/g,a3=/\s+/,aG=/\r/g,a2=/^(?:href|src|style)$/,f=/^(?:button|input)$/i,C=/^(?:button|input|object|select|textarea)$/i,k=/^a(?:rea)?$/i,Q=/^(?:radio|checkbox)$/i;a.props={"for":"htmlFor","class":"className",readonly:"readOnly",maxlength:"maxLength",cellspacing:"cellSpacing",rowspan:"rowSpan",colspan:"colSpan",tabindex:"tabIndex",usemap:"useMap",frameborder:"frameBorder"};a.fn.extend({attr:function(e,bd){return a.access(this,e,bd,true,a.attr)},removeAttr:function(e,bd){return this.each(function(){a.attr(this,e,"");if(this.nodeType===1){this.removeAttribute(e)}})},addClass:function(bj){if(a.isFunction(bj)){return this.each(function(bm){var bl=a(this);bl.addClass(bj.call(this,bm,bl.attr("class")))})}if(bj&&typeof bj==="string"){var e=(bj||"").split(a3);for(var bf=0,be=this.length;bf<be;bf++){var bd=this[bf];if(bd.nodeType===1){if(!bd.className){bd.className=bj}else{var bg=" "+bd.className+" ",bi=bd.className;for(var bh=0,bk=e.length;bh<bk;bh++){if(bg.indexOf(" "+e[bh]+" ")<0){bi+=" "+e[bh]}}bd.className=a.trim(bi)}}}}return this},removeClass:function(bh){if(a.isFunction(bh)){return this.each(function(bl){var bk=a(this);bk.removeClass(bh.call(this,bl,bk.attr("class")))})}if((bh&&typeof bh==="string")||bh===H){var bi=(bh||"").split(a3);for(var be=0,bd=this.length;be<bd;be++){var bg=this[be];if(bg.nodeType===1&&bg.className){if(bh){var bf=(" "+bg.className+" ").replace(aC," ");for(var bj=0,e=bi.length;bj<e;bj++){bf=bf.replace(" "+bi[bj]+" "," ")}bg.className=a.trim(bf)}else{bg.className=""}}}}return this},toggleClass:function(bf,bd){var be=typeof bf,e=typeof bd==="boolean";if(a.isFunction(bf)){return this.each(function(bh){var bg=a(this);bg.toggleClass(bf.call(this,bh,bg.attr("class"),bd),bd)})}return this.each(function(){if(be==="string"){var bi,bh=0,bg=a(this),bj=bd,bk=bf.split(a3);while((bi=bk[bh++])){bj=e?bj:!bg.hasClass(bi);bg[bj?"addClass":"removeClass"](bi)}}else{if(be==="undefined"||be==="boolean"){if(this.className){a._data(this,"__className__",this.className)}this.className=this.className||bf===false?"":a._data(this,"__className__")||""}}})},hasClass:function(e){var bf=" "+e+" ";for(var be=0,bd=this.length;be<bd;be++){if((" "+this[be].className+" ").replace(aC," ").indexOf(bf)>-1){return true}}return false},val:function(bk){if(!arguments.length){var be=this[0];if(be){if(a.nodeName(be,"option")){var bd=be.attributes.value;return !bd||bd.specified?be.value:be.text}if(a.nodeName(be,"select")){var bi=be.selectedIndex,bl=[],bm=be.options,bh=be.type==="select-one";if(bi<0){return null}for(var bf=bh?bi:0,bj=bh?bi+1:bm.length;bf<bj;bf++){var bg=bm[bf];if(bg.selected&&(a.support.optDisabled?!bg.disabled:bg.getAttribute("disabled")===null)&&(!bg.parentNode.disabled||!a.nodeName(bg.parentNode,"optgroup"))){bk=a(bg).val();if(bh){return bk}bl.push(bk)}}if(bh&&!bl.length&&bm.length){return a(bm[bi]).val()}return bl}if(Q.test(be.type)&&!a.support.checkOn){return be.getAttribute("value")===null?"on":be.value}return(be.value||"").replace(aG,"")}return H}var e=a.isFunction(bk);return this.each(function(bp){var bo=a(this),bq=bk;if(this.nodeType!==1){return}if(e){bq=bk.call(this,bp,bo.val())}if(bq==null){bq=""}else{if(typeof bq==="number"){bq+=""}else{if(a.isArray(bq)){bq=a.map(bq,function(br){return br==null?"":br+""})}}}if(a.isArray(bq)&&Q.test(this.type)){this.checked=a.inArray(bo.val(),bq)>=0}else{if(a.nodeName(this,"select")){var bn=a.makeArray(bq);a("option",this).each(function(){this.selected=a.inArray(a(this).val(),bn)>=0});if(!bn.length){this.selectedIndex=-1}}else{this.value=bq}}})}});a.extend({attrFn:{val:true,css:true,html:true,text:true,data:true,width:true,height:true,offset:true},attr:function(bd,e,bi,bl){if(!bd||bd.nodeType===3||bd.nodeType===8||bd.nodeType===2){return H}if(bl&&e in a.attrFn){return a(bd)[e](bi)}var be=bd.nodeType!==1||!a.isXMLDoc(bd),bh=bi!==H;e=be&&a.props[e]||e;if(bd.nodeType===1){var bg=a2.test(e);if(e==="selected"&&!a.support.optSelected){var bj=bd.parentNode;if(bj){bj.selectedIndex;if(bj.parentNode){bj.parentNode.selectedIndex}}}if((e in bd||bd[e]!==H)&&be&&!bg){if(bh){if(e==="type"&&f.test(bd.nodeName)&&bd.parentNode){a.error("type property can't be changed")}if(bi===null){if(bd.nodeType===1){bd.removeAttribute(e)}}else{bd[e]=bi}}if(a.nodeName(bd,"form")&&bd.getAttributeNode(e)){return bd.getAttributeNode(e).nodeValue}if(e==="tabIndex"){var bk=bd.getAttributeNode("tabIndex");return bk&&bk.specified?bk.value:C.test(bd.nodeName)||k.test(bd.nodeName)&&bd.href?0:H}return bd[e]}if(!a.support.style&&be&&e==="style"){if(bh){bd.style.cssText=""+bi}return bd.style.cssText}if(bh){bd.setAttribute(e,""+bi)}if(!bd.attributes[e]&&(bd.hasAttribute&&!bd.hasAttribute(e))){return H}var bf=!a.support.hrefNormalized&&be&&bg?bd.getAttribute(e,2):bd.getAttribute(e);return bf===null?H:bf}if(bh){bd[e]=bi}return bd[e]}});var aP=/\.(.*)$/,a0=/^(?:textarea|input|select)$/i,K=/\./g,aa=/ /g,aw=/[^\w\s.|`]/g,E=function(e){return e.replace(aw,"\\$&")};a.event={add:function(bg,bk,br,bi){if(bg.nodeType===3||bg.nodeType===8){return}try{if(a.isWindow(bg)&&(bg!==aY&&!bg.frameElement)){bg=aY}}catch(bl){}if(br===false){br=a5}else{if(!br){return}}var be,bp;if(br.handler){be=br;br=be.handler}if(!br.guid){br.guid=a.guid++}var bm=a._data(bg);if(!bm){return}var bq=bm.events,bj=bm.handle;if(!bq){bm.events=bq={}}if(!bj){bm.handle=bj=function(){return typeof a!=="undefined"&&!a.event.triggered?a.event.handle.apply(bj.elem,arguments):H}}bj.elem=bg;bk=bk.split(" ");var bo,bh=0,bd;while((bo=bk[bh++])){bp=be?a.extend({},be):{handler:br,data:bi};if(bo.indexOf(".")>-1){bd=bo.split(".");bo=bd.shift();bp.namespace=bd.slice(0).sort().join(".")}else{bd=[];bp.namespace=""}bp.type=bo;if(!bp.guid){bp.guid=br.guid}var bf=bq[bo],bn=a.event.special[bo]||{};if(!bf){bf=bq[bo]=[];if(!bn.setup||bn.setup.call(bg,bi,bd,bj)===false){if(bg.addEventListener){bg.addEventListener(bo,bj,false)}else{if(bg.attachEvent){bg.attachEvent("on"+bo,bj)}}}}if(bn.add){bn.add.call(bg,bp);if(!bp.handler.guid){bp.handler.guid=br.guid}}bf.push(bp);a.event.global[bo]=true}bg=null},global:{},remove:function(br,bm,be,bi){if(br.nodeType===3||br.nodeType===8){return}if(be===false){be=a5}var bu,bh,bj,bo,bp=0,bf,bk,bn,bg,bl,e,bt,bq=a.hasData(br)&&a._data(br),bd=bq&&bq.events;if(!bq||!bd){return}if(bm&&bm.type){be=bm.handler;bm=bm.type}if(!bm||typeof bm==="string"&&bm.charAt(0)==="."){bm=bm||"";for(bh in bd){a.event.remove(br,bh+bm)}return}bm=bm.split(" ");while((bh=bm[bp++])){bt=bh;e=null;bf=bh.indexOf(".")<0;bk=[];if(!bf){bk=bh.split(".");bh=bk.shift();bn=new RegExp("(^|\\.)"+a.map(bk.slice(0).sort(),E).join("\\.(?:.*\\.)?")+"(\\.|$)")}bl=bd[bh];if(!bl){continue}if(!be){for(bo=0;bo<bl.length;bo++){e=bl[bo];if(bf||bn.test(e.namespace)){a.event.remove(br,bt,e.handler,bo);bl.splice(bo--,1)}}continue}bg=a.event.special[bh]||{};for(bo=bi||0;bo<bl.length;bo++){e=bl[bo];if(be.guid===e.guid){if(bf||bn.test(e.namespace)){if(bi==null){bl.splice(bo--,1)}if(bg.remove){bg.remove.call(br,e)}}if(bi!=null){break}}}if(bl.length===0||bi!=null&&bl.length===1){if(!bg.teardown||bg.teardown.call(br,bk)===false){a.removeEvent(br,bh,bq.handle)}bu=null;delete bd[bh]}}if(a.isEmptyObject(bd)){var bs=bq.handle;if(bs){bs.elem=null}delete bq.events;delete bq.handle;if(a.isEmptyObject(bq)){a.removeData(br,H,true)}}},trigger:function(bd,bi,bf){var bm=bd.type||bd,bh=arguments[3];if(!bh){bd=typeof bd==="object"?bd[a.expando]?bd:a.extend(a.Event(bm),bd):a.Event(bm);if(bm.indexOf("!")>=0){bd.type=bm=bm.slice(0,-1);bd.exclusive=true}if(!bf){bd.stopPropagation();if(a.event.global[bm]){a.each(a.cache,function(){var br=a.expando,bq=this[br];if(bq&&bq.events&&bq.events[bm]){a.event.trigger(bd,bi,bq.handle.elem)}})}}if(!bf||bf.nodeType===3||bf.nodeType===8){return H}bd.result=H;bd.target=bf;bi=a.makeArray(bi);bi.unshift(bd)}bd.currentTarget=bf;var bj=a._data(bf,"handle");if(bj){bj.apply(bf,bi)}var bo=bf.parentNode||bf.ownerDocument;try{if(!(bf&&bf.nodeName&&a.noData[bf.nodeName.toLowerCase()])){if(bf["on"+bm]&&bf["on"+bm].apply(bf,bi)===false){bd.result=false;bd.preventDefault()}}}catch(bn){}if(!bd.isPropagationStopped()&&bo){a.event.trigger(bd,bi,bo,true)}else{if(!bd.isDefaultPrevented()){var be,bk=bd.target,e=bm.replace(aP,""),bp=a.nodeName(bk,"a")&&e==="click",bl=a.event.special[e]||{};if((!bl._default||bl._default.call(bf,bd)===false)&&!bp&&!(bk&&bk.nodeName&&a.noData[bk.nodeName.toLowerCase()])){try{if(bk[e]){be=bk["on"+e];if(be){bk["on"+e]=null}a.event.triggered=true;bk[e]()}}catch(bg){}if(be){bk["on"+e]=be}a.event.triggered=false}}}},handle:function(e){var bl,be,bd,bn,bm,bh=[],bj=a.makeArray(arguments);e=bj[0]=a.event.fix(e||aY.event);e.currentTarget=this;bl=e.type.indexOf(".")<0&&!e.exclusive;if(!bl){bd=e.type.split(".");e.type=bd.shift();bh=bd.slice(0).sort();bn=new RegExp("(^|\\.)"+bh.join("\\.(?:.*\\.)?")+"(\\.|$)")}e.namespace=e.namespace||bh.join(".");bm=a._data(this,"events");be=(bm||{})[e.type];if(bm&&be){be=be.slice(0);for(var bg=0,bf=be.length;bg<bf;bg++){var bk=be[bg];if(bl||bn.test(bk.namespace)){e.handler=bk.handler;e.data=bk.data;e.handleObj=bk;var bi=bk.handler.apply(this,bj);if(bi!==H){e.result=bi;if(bi===false){e.preventDefault();e.stopPropagation()}}if(e.isImmediatePropagationStopped()){break}}}}return e.result},props:"altKey attrChange attrName bubbles button cancelable charCode clientX clientY ctrlKey currentTarget data detail eventPhase fromElement handler keyCode layerX layerY metaKey newValue offsetX offsetY pageX pageY prevValue relatedNode relatedTarget screenX screenY shiftKey srcElement target toElement view wheelDelta which".split(" "),fix:function(bf){if(bf[a.expando]){return bf}var bd=bf;bf=a.Event(bd);for(var be=this.props.length,bh;be;){bh=this.props[--be];bf[bh]=bd[bh]}if(!bf.target){bf.target=bf.srcElement||al}if(bf.target.nodeType===3){bf.target=bf.target.parentNode}if(!bf.relatedTarget&&bf.fromElement){bf.relatedTarget=bf.fromElement===bf.target?bf.toElement:bf.fromElement}if(bf.pageX==null&&bf.clientX!=null){var bg=al.documentElement,e=al.body;bf.pageX=bf.clientX+(bg&&bg.scrollLeft||e&&e.scrollLeft||0)-(bg&&bg.clientLeft||e&&e.clientLeft||0);bf.pageY=bf.clientY+(bg&&bg.scrollTop||e&&e.scrollTop||0)-(bg&&bg.clientTop||e&&e.clientTop||0)}if(bf.which==null&&(bf.charCode!=null||bf.keyCode!=null)){bf.which=bf.charCode!=null?bf.charCode:bf.keyCode}if(!bf.metaKey&&bf.ctrlKey){bf.metaKey=bf.ctrlKey}if(!bf.which&&bf.button!==H){bf.which=(bf.button&1?1:(bf.button&2?3:(bf.button&4?2:0)))}return bf},guid:100000000,proxy:a.proxy,special:{ready:{setup:a.bindReady,teardown:a.noop},live:{add:function(e){a.event.add(this,n(e.origType,e.selector),a.extend({},e,{handler:af,guid:e.handler.guid}))},remove:function(e){a.event.remove(this,n(e.origType,e.selector),e)}},beforeunload:{setup:function(be,bd,e){if(a.isWindow(this)){this.onbeforeunload=e}},teardown:function(bd,e){if(this.onbeforeunload===e){this.onbeforeunload=null}}}}};a.removeEvent=al.removeEventListener?function(bd,e,be){if(bd.removeEventListener){bd.removeEventListener(e,be,false)}}:function(bd,e,be){if(bd.detachEvent){bd.detachEvent("on"+e,be)}};a.Event=function(e){if(!this.preventDefault){return new a.Event(e)}if(e&&e.type){this.originalEvent=e;this.type=e.type;this.isDefaultPrevented=(e.defaultPrevented||e.returnValue===false||e.getPreventDefault&&e.getPreventDefault())?h:a5}else{this.type=e}this.timeStamp=a.now();this[a.expando]=true};function a5(){return false}function h(){return true}a.Event.prototype={preventDefault:function(){this.isDefaultPrevented=h;var bd=this.originalEvent;if(!bd){return}if(bd.preventDefault){bd.preventDefault()}else{bd.returnValue=false}},stopPropagation:function(){this.isPropagationStopped=h;var bd=this.originalEvent;if(!bd){return}if(bd.stopPropagation){bd.stopPropagation()}bd.cancelBubble=true},stopImmediatePropagation:function(){this.isImmediatePropagationStopped=h;this.stopPropagation()},isDefaultPrevented:a5,isPropagationStopped:a5,isImmediatePropagationStopped:a5};var Z=function(be){var bd=be.relatedTarget;try{if(bd!==al&&!bd.parentNode){return}while(bd&&bd!==this){bd=bd.parentNode}if(bd!==this){be.type=be.data;a.event.handle.apply(this,arguments)}}catch(bf){}},aK=function(e){e.type=e.data;a.event.handle.apply(this,arguments)};a.each({mouseenter:"mouseover",mouseleave:"mouseout"},function(bd,e){a.event.special[bd]={setup:function(be){a.event.add(this,e,be&&be.selector?aK:Z,bd)},teardown:function(be){a.event.remove(this,e,be&&be.selector?aK:Z)}}});if(!a.support.submitBubbles){a.event.special.submit={setup:function(bd,e){if(this.nodeName&&this.nodeName.toLowerCase()!=="form"){a.event.add(this,"click.specialSubmit",function(bg){var bf=bg.target,be=bf.type;if((be==="submit"||be==="image")&&a(bf).closest("form").length){aN("submit",this,arguments)}});a.event.add(this,"keypress.specialSubmit",function(bg){var bf=bg.target,be=bf.type;if((be==="text"||be==="password")&&a(bf).closest("form").length&&bg.keyCode===13){aN("submit",this,arguments)}})}else{return false}},teardown:function(e){a.event.remove(this,".specialSubmit")}}}if(!a.support.changeBubbles){var a6,j=function(bd){var e=bd.type,be=bd.value;if(e==="radio"||e==="checkbox"){be=bd.checked}else{if(e==="select-multiple"){be=bd.selectedIndex>-1?a.map(bd.options,function(bf){return bf.selected}).join("-"):""}else{if(bd.nodeName.toLowerCase()==="select"){be=bd.selectedIndex}}}return be},X=function X(bf){var bd=bf.target,be,bg;if(!a0.test(bd.nodeName)||bd.readOnly){return}be=a._data(bd,"_change_data");bg=j(bd);if(bf.type!=="focusout"||bd.type!=="radio"){a._data(bd,"_change_data",bg)}if(be===H||bg===be){return}if(be!=null||bg){bf.type="change";bf.liveFired=H;a.event.trigger(bf,arguments[1],bd)}};a.event.special.change={filters:{focusout:X,beforedeactivate:X,click:function(bf){var be=bf.target,bd=be.type;if(bd==="radio"||bd==="checkbox"||be.nodeName.toLowerCase()==="select"){X.call(this,bf)}},keydown:function(bf){var be=bf.target,bd=be.type;if((bf.keyCode===13&&be.nodeName.toLowerCase()!=="textarea")||(bf.keyCode===32&&(bd==="checkbox"||bd==="radio"))||bd==="select-multiple"){X.call(this,bf)}},beforeactivate:function(be){var bd=be.target;a._data(bd,"_change_data",j(bd))}},setup:function(be,bd){if(this.type==="file"){return false}for(var e in a6){a.event.add(this,e+".specialChange",a6[e])}return a0.test(this.nodeName)},teardown:function(e){a.event.remove(this,".specialChange");return a0.test(this.nodeName)}};a6=a.event.special.change.filters;a6.focus=a6.beforeactivate}function aN(bd,bf,e){var be=a.extend({},e[0]);be.type=bd;be.originalEvent={};be.liveFired=H;a.event.handle.call(bf,be);if(be.isDefaultPrevented()){e[0].preventDefault()}}if(al.addEventListener){a.each({focus:"focusin",blur:"focusout"},function(be,e){a.event.special[e]={setup:function(){this.addEventListener(be,bd,true)},teardown:function(){this.removeEventListener(be,bd,true)}};function bd(bf){bf=a.event.fix(bf);bf.type=e;return a.event.handle.call(this,bf)}})}a.each(["bind","one"],function(bd,e){a.fn[e]=function(bj,bk,bi){if(typeof bj==="object"){for(var bg in bj){this[e](bg,bk,bj[bg],bi)}return this}if(a.isFunction(bk)||bk===false){bi=bk;bk=H}var bh=e==="one"?a.proxy(bi,function(bl){a(this).unbind(bl,bh);return bi.apply(this,arguments)}):bi;if(bj==="unload"&&e!=="one"){this.one(bj,bk,bi)}else{for(var bf=0,be=this.length;bf<be;bf++){a.event.add(this[bf],bj,bh,bk)}}return this}});a.fn.extend({unbind:function(bg,bf){if(typeof bg==="object"&&!bg.preventDefault){for(var be in bg){this.unbind(be,bg[be])}}else{for(var bd=0,e=this.length;bd<e;bd++){a.event.remove(this[bd],bg,bf)}}return this},delegate:function(e,bd,bf,be){return this.live(bd,bf,be,e)},undelegate:function(e,bd,be){if(arguments.length===0){return this.unbind("live")}else{return this.die(bd,null,be,e)}},trigger:function(e,bd){return this.each(function(){a.event.trigger(e,bd,this)})},triggerHandler:function(e,be){if(this[0]){var bd=a.Event(e);bd.preventDefault();bd.stopPropagation();a.event.trigger(bd,be,this[0]);return bd.result}},toggle:function(be){var e=arguments,bd=1;while(bd<e.length){a.proxy(be,e[bd++])}return this.click(a.proxy(be,function(bf){var bg=(a._data(this,"lastToggle"+be.guid)||0)%bd;a._data(this,"lastToggle"+be.guid,bg+1);bf.preventDefault();return e[bg].apply(this,arguments)||false}))},hover:function(e,bd){return this.mouseenter(e).mouseleave(bd||e)}});var aH={focus:"focusin",blur:"focusout",mouseenter:"mouseover",mouseleave:"mouseout"};a.each(["live","die"],function(bd,e){a.fn[e]=function(bn,bk,bp,bg){var bo,bl=0,bm,bf,br,bi=bg||this.selector,be=bg?this:a(this.context);if(typeof bn==="object"&&!bn.preventDefault){for(var bq in bn){be[e](bq,bk,bn[bq],bi)}return this}if(a.isFunction(bk)){bp=bk;bk=H}bn=(bn||"").split(" ");while((bo=bn[bl++])!=null){bm=aP.exec(bo);bf="";if(bm){bf=bm[0];bo=bo.replace(aP,"")}if(bo==="hover"){bn.push("mouseenter"+bf,"mouseleave"+bf);continue}br=bo;if(bo==="focus"||bo==="blur"){bn.push(aH[bo]+bf);bo=bo+bf}else{bo=(aH[bo]||bo)+bf}if(e==="live"){for(var bj=0,bh=be.length;bj<bh;bj++){a.event.add(be[bj],"live."+n(bo,bi),{data:bk,selector:bi,handler:bp,origType:bo,origHandler:bp,preType:br})}}else{be.unbind("live."+n(bo,bi),bp)}}return this}});function af(bn){var bk,bf,bt,bh,e,bp,bm,bo,bl,bs,bj,bi,br,bq=[],bg=[],bd=a._data(this,"events");if(bn.liveFired===this||!bd||!bd.live||bn.target.disabled||bn.button&&bn.type==="click"){return}if(bn.namespace){bi=new RegExp("(^|\\.)"+bn.namespace.split(".").join("\\.(?:.*\\.)?")+"(\\.|$)")}bn.liveFired=this;var be=bd.live.slice(0);for(bm=0;bm<be.length;bm++){e=be[bm];if(e.origType.replace(aP,"")===bn.type){bg.push(e.selector)}else{be.splice(bm--,1)}}bh=a(bn.target).closest(bg,bn.currentTarget);for(bo=0,bl=bh.length;bo<bl;bo++){bj=bh[bo];for(bm=0;bm<be.length;bm++){e=be[bm];if(bj.selector===e.selector&&(!bi||bi.test(e.namespace))&&!bj.elem.disabled){bp=bj.elem;bt=null;if(e.preType==="mouseenter"||e.preType==="mouseleave"){bn.type=e.preType;bt=a(bn.relatedTarget).closest(e.selector)[0]}if(!bt||bt!==bp){bq.push({elem:bp,handleObj:e,level:bj.level})}}}}for(bo=0,bl=bq.length;bo<bl;bo++){bh=bq[bo];if(bf&&bh.level>bf){break}bn.currentTarget=bh.elem;bn.data=bh.handleObj.data;bn.handleObj=bh.handleObj;br=bh.handleObj.origHandler.apply(bh.elem,arguments);if(br===false||bn.isPropagationStopped()){bf=bh.level;if(br===false){bk=false}if(bn.isImmediatePropagationStopped()){break}}}return bk}function n(bd,e){return(bd&&bd!=="*"?bd+".":"")+e.replace(K,"`").replace(aa,"&")}a.each(("blur focus focusin focusout load resize scroll unload click dblclick mousedown mouseup mousemove mouseover mouseout mouseenter mouseleave change select submit keydown keypress keyup error").split(" "),function(bd,e){a.fn[e]=function(bf,be){if(be==null){be=bf;bf=null}return arguments.length>0?this.bind(e,bf,be):this.trigger(e)};if(a.attrFn){a.attrFn[e]=true}});
/*
 * Sizzle CSS Selector Engine
 *  Copyright 2011, The Dojo Foundation
 *  Released under the MIT, BSD, and GPL Licenses.
 *  More information: http://sizzlejs.com/
 */
(function(){var bn=/((?:\((?:\([^()]+\)|[^()]+)+\)|\[(?:\[[^\[\]]*\]|['"][^'"]*['"]|[^\[\]'"]+)+\]|\\.|[^ >+~,(\[\\]+)+|[>+~])(\s*,\s*)?((?:.|\r|\n)*)/g,bo=0,br=Object.prototype.toString,bi=false,bh=true,bp=/\\/g,bv=/\W/;[0,0].sort(function(){bh=false;return 0});var bf=function(bA,e,bD,bE){bD=bD||[];e=e||al;var bG=e;if(e.nodeType!==1&&e.nodeType!==9){return[]}if(!bA||typeof bA!=="string"){return bD}var bx,bI,bL,bw,bH,bK,bJ,bC,bz=true,by=bf.isXML(e),bB=[],bF=bA;do{bn.exec("");bx=bn.exec(bF);if(bx){bF=bx[3];bB.push(bx[1]);if(bx[2]){bw=bx[3];break}}}while(bx);if(bB.length>1&&bj.exec(bA)){if(bB.length===2&&bk.relative[bB[0]]){bI=bs(bB[0]+bB[1],e)}else{bI=bk.relative[bB[0]]?[e]:bf(bB.shift(),e);while(bB.length){bA=bB.shift();if(bk.relative[bA]){bA+=bB.shift()}bI=bs(bA,bI)}}}else{if(!bE&&bB.length>1&&e.nodeType===9&&!by&&bk.match.ID.test(bB[0])&&!bk.match.ID.test(bB[bB.length-1])){bH=bf.find(bB.shift(),e,by);e=bH.expr?bf.filter(bH.expr,bH.set)[0]:bH.set[0]}if(e){bH=bE?{expr:bB.pop(),set:bl(bE)}:bf.find(bB.pop(),bB.length===1&&(bB[0]==="~"||bB[0]==="+")&&e.parentNode?e.parentNode:e,by);bI=bH.expr?bf.filter(bH.expr,bH.set):bH.set;if(bB.length>0){bL=bl(bI)}else{bz=false}while(bB.length){bK=bB.pop();bJ=bK;if(!bk.relative[bK]){bK=""}else{bJ=bB.pop()}if(bJ==null){bJ=e}bk.relative[bK](bL,bJ,by)}}else{bL=bB=[]}}if(!bL){bL=bI}if(!bL){bf.error(bK||bA)}if(br.call(bL)==="[object Array]"){if(!bz){bD.push.apply(bD,bL)}else{if(e&&e.nodeType===1){for(bC=0;bL[bC]!=null;bC++){if(bL[bC]&&(bL[bC]===true||bL[bC].nodeType===1&&bf.contains(e,bL[bC]))){bD.push(bI[bC])}}}else{for(bC=0;bL[bC]!=null;bC++){if(bL[bC]&&bL[bC].nodeType===1){bD.push(bI[bC])}}}}}else{bl(bL,bD)}if(bw){bf(bw,bG,bD,bE);bf.uniqueSort(bD)}return bD};bf.uniqueSort=function(bw){if(bq){bi=bh;bw.sort(bq);if(bi){for(var e=1;e<bw.length;e++){if(bw[e]===bw[e-1]){bw.splice(e--,1)}}}}return bw};bf.matches=function(e,bw){return bf(e,null,null,bw)};bf.matchesSelector=function(e,bw){return bf(bw,null,null,[e]).length>0};bf.find=function(bC,e,bD){var bB;if(!bC){return[]}for(var by=0,bx=bk.order.length;by<bx;by++){var bz,bA=bk.order[by];if((bz=bk.leftMatch[bA].exec(bC))){var bw=bz[1];bz.splice(1,1);if(bw.substr(bw.length-1)!=="\\"){bz[1]=(bz[1]||"").replace(bp,"");bB=bk.find[bA](bz,e,bD);if(bB!=null){bC=bC.replace(bk.match[bA],"");break}}}}if(!bB){bB=typeof e.getElementsByTagName!=="undefined"?e.getElementsByTagName("*"):[]}return{set:bB,expr:bC}};bf.filter=function(bG,bF,bJ,bz){var bB,e,bx=bG,bL=[],bD=bF,bC=bF&&bF[0]&&bf.isXML(bF[0]);while(bG&&bF.length){for(var bE in bk.filter){if((bB=bk.leftMatch[bE].exec(bG))!=null&&bB[2]){var bK,bI,bw=bk.filter[bE],by=bB[1];e=false;bB.splice(1,1);if(by.substr(by.length-1)==="\\"){continue}if(bD===bL){bL=[]}if(bk.preFilter[bE]){bB=bk.preFilter[bE](bB,bD,bJ,bL,bz,bC);if(!bB){e=bK=true}else{if(bB===true){continue}}}if(bB){for(var bA=0;(bI=bD[bA])!=null;bA++){if(bI){bK=bw(bI,bB,bA,bD);var bH=bz^!!bK;if(bJ&&bK!=null){if(bH){e=true}else{bD[bA]=false}}else{if(bH){bL.push(bI);e=true}}}}}if(bK!==H){if(!bJ){bD=bL}bG=bG.replace(bk.match[bE],"");if(!e){return[]}break}}}if(bG===bx){if(e==null){bf.error(bG)}else{break}}bx=bG}return bD};bf.error=function(e){throw"Syntax error, unrecognized expression: "+e};var bk=bf.selectors={order:["ID","NAME","TAG"],match:{ID:/#((?:[\w\u00c0-\uFFFF\-]|\\.)+)/,CLASS:/\.((?:[\w\u00c0-\uFFFF\-]|\\.)+)/,NAME:/\[name=['"]*((?:[\w\u00c0-\uFFFF\-]|\\.)+)['"]*\]/,ATTR:/\[\s*((?:[\w\u00c0-\uFFFF\-]|\\.)+)\s*(?:(\S?=)\s*(?:(['"])(.*?)\3|(#?(?:[\w\u00c0-\uFFFF\-]|\\.)*)|)|)\s*\]/,TAG:/^((?:[\w\u00c0-\uFFFF\*\-]|\\.)+)/,CHILD:/:(only|nth|last|first)-child(?:\(\s*(even|odd|(?:[+\-]?\d+|(?:[+\-]?\d*)?n\s*(?:[+\-]\s*\d+)?))\s*\))?/,POS:/:(nth|eq|gt|lt|first|last|even|odd)(?:\((\d*)\))?(?=[^\-]|$)/,PSEUDO:/:((?:[\w\u00c0-\uFFFF\-]|\\.)+)(?:\((['"]?)((?:\([^\)]+\)|[^\(\)]*)+)\2\))?/},leftMatch:{},attrMap:{"class":"className","for":"htmlFor"},attrHandle:{href:function(e){return e.getAttribute("href")},type:function(e){return e.getAttribute("type")}},relative:{"+":function(bB,bw){var by=typeof bw==="string",bA=by&&!bv.test(bw),bC=by&&!bA;if(bA){bw=bw.toLowerCase()}for(var bx=0,e=bB.length,bz;bx<e;bx++){if((bz=bB[bx])){while((bz=bz.previousSibling)&&bz.nodeType!==1){}bB[bx]=bC||bz&&bz.nodeName.toLowerCase()===bw?bz||false:bz===bw}}if(bC){bf.filter(bw,bB,true)}},">":function(bB,bw){var bA,bz=typeof bw==="string",bx=0,e=bB.length;if(bz&&!bv.test(bw)){bw=bw.toLowerCase();for(;bx<e;bx++){bA=bB[bx];if(bA){var by=bA.parentNode;bB[bx]=by.nodeName.toLowerCase()===bw?by:false}}}else{for(;bx<e;bx++){bA=bB[bx];if(bA){bB[bx]=bz?bA.parentNode:bA.parentNode===bw}}if(bz){bf.filter(bw,bB,true)}}},"":function(by,bw,bA){var bz,bx=bo++,e=bt;if(typeof bw==="string"&&!bv.test(bw)){bw=bw.toLowerCase();bz=bw;e=bd}e("parentNode",bw,bx,by,bz,bA)},"~":function(by,bw,bA){var bz,bx=bo++,e=bt;if(typeof bw==="string"&&!bv.test(bw)){bw=bw.toLowerCase();bz=bw;e=bd}e("previousSibling",bw,bx,by,bz,bA)}},find:{ID:function(bw,bx,by){if(typeof bx.getElementById!=="undefined"&&!by){var e=bx.getElementById(bw[1]);return e&&e.parentNode?[e]:[]}},NAME:function(bx,bA){if(typeof bA.getElementsByName!=="undefined"){var bw=[],bz=bA.getElementsByName(bx[1]);for(var by=0,e=bz.length;by<e;by++){if(bz[by].getAttribute("name")===bx[1]){bw.push(bz[by])}}return bw.length===0?null:bw}},TAG:function(e,bw){if(typeof bw.getElementsByTagName!=="undefined"){return bw.getElementsByTagName(e[1])}}},preFilter:{CLASS:function(by,bw,bx,e,bB,bC){by=" "+by[1].replace(bp,"")+" ";if(bC){return by}for(var bz=0,bA;(bA=bw[bz])!=null;bz++){if(bA){if(bB^(bA.className&&(" "+bA.className+" ").replace(/[\t\n\r]/g," ").indexOf(by)>=0)){if(!bx){e.push(bA)}}else{if(bx){bw[bz]=false}}}}return false},ID:function(e){return e[1].replace(bp,"")},TAG:function(bw,e){return bw[1].replace(bp,"").toLowerCase()},CHILD:function(e){if(e[1]==="nth"){if(!e[2]){bf.error(e[0])}e[2]=e[2].replace(/^\+|\s*/g,"");var bw=/(-?)(\d*)(?:n([+\-]?\d*))?/.exec(e[2]==="even"&&"2n"||e[2]==="odd"&&"2n+1"||!/\D/.test(e[2])&&"0n+"+e[2]||e[2]);e[2]=(bw[1]+(bw[2]||1))-0;e[3]=bw[3]-0}else{if(e[2]){bf.error(e[0])}}e[0]=bo++;return e},ATTR:function(bz,bw,bx,e,bA,bB){var by=bz[1]=bz[1].replace(bp,"");if(!bB&&bk.attrMap[by]){bz[1]=bk.attrMap[by]}bz[4]=(bz[4]||bz[5]||"").replace(bp,"");if(bz[2]==="~="){bz[4]=" "+bz[4]+" "}return bz},PSEUDO:function(bz,bw,bx,e,bA){if(bz[1]==="not"){if((bn.exec(bz[3])||"").length>1||/^\w/.test(bz[3])){bz[3]=bf(bz[3],null,null,bw)}else{var by=bf.filter(bz[3],bw,bx,true^bA);if(!bx){e.push.apply(e,by)}return false}}else{if(bk.match.POS.test(bz[0])||bk.match.CHILD.test(bz[0])){return true}}return bz},POS:function(e){e.unshift(true);return e}},filters:{enabled:function(e){return e.disabled===false&&e.type!=="hidden"},disabled:function(e){return e.disabled===true},checked:function(e){return e.checked===true},selected:function(e){if(e.parentNode){e.parentNode.selectedIndex}return e.selected===true},parent:function(e){return !!e.firstChild},empty:function(e){return !e.firstChild},has:function(bx,bw,e){return !!bf(e[3],bx).length},header:function(e){return(/h\d/i).test(e.nodeName)},text:function(e){return"text"===e.getAttribute("type")},radio:function(e){return"radio"===e.type},checkbox:function(e){return"checkbox"===e.type},file:function(e){return"file"===e.type},password:function(e){return"password"===e.type},submit:function(e){return"submit"===e.type},image:function(e){return"image"===e.type},reset:function(e){return"reset"===e.type},button:function(e){return"button"===e.type||e.nodeName.toLowerCase()==="button"},input:function(e){return(/input|select|textarea|button/i).test(e.nodeName)}},setFilters:{first:function(bw,e){return e===0},last:function(bx,bw,e,by){return bw===by.length-1},even:function(bw,e){return e%2===0},odd:function(bw,e){return e%2===1},lt:function(bx,bw,e){return bw<e[3]-0},gt:function(bx,bw,e){return bw>e[3]-0},nth:function(bx,bw,e){return e[3]-0===bw},eq:function(bx,bw,e){return e[3]-0===bw}},filter:{PSEUDO:function(bx,bC,bB,bD){var e=bC[1],bw=bk.filters[e];if(bw){return bw(bx,bB,bC,bD)}else{if(e==="contains"){return(bx.textContent||bx.innerText||bf.getText([bx])||"").indexOf(bC[3])>=0}else{if(e==="not"){var by=bC[3];for(var bA=0,bz=by.length;bA<bz;bA++){if(by[bA]===bx){return false}}return true}else{bf.error(e)}}}},CHILD:function(e,by){var bB=by[1],bw=e;switch(bB){case"only":case"first":while((bw=bw.previousSibling)){if(bw.nodeType===1){return false}}if(bB==="first"){return true}bw=e;case"last":while((bw=bw.nextSibling)){if(bw.nodeType===1){return false}}return true;case"nth":var bx=by[2],bE=by[3];if(bx===1&&bE===0){return true}var bA=by[0],bD=e.parentNode;if(bD&&(bD.sizcache!==bA||!e.nodeIndex)){var bz=0;for(bw=bD.firstChild;bw;bw=bw.nextSibling){if(bw.nodeType===1){bw.nodeIndex=++bz}}bD.sizcache=bA}var bC=e.nodeIndex-bE;if(bx===0){return bC===0}else{return(bC%bx===0&&bC/bx>=0)}}},ID:function(bw,e){return bw.nodeType===1&&bw.getAttribute("id")===e},TAG:function(bw,e){return(e==="*"&&bw.nodeType===1)||bw.nodeName.toLowerCase()===e},CLASS:function(bw,e){return(" "+(bw.className||bw.getAttribute("class"))+" ").indexOf(e)>-1},ATTR:function(bA,by){var bx=by[1],e=bk.attrHandle[bx]?bk.attrHandle[bx](bA):bA[bx]!=null?bA[bx]:bA.getAttribute(bx),bB=e+"",bz=by[2],bw=by[4];return e==null?bz==="!=":bz==="="?bB===bw:bz==="*="?bB.indexOf(bw)>=0:bz==="~="?(" "+bB+" ").indexOf(bw)>=0:!bw?bB&&e!==false:bz==="!="?bB!==bw:bz==="^="?bB.indexOf(bw)===0:bz==="$="?bB.substr(bB.length-bw.length)===bw:bz==="|="?bB===bw||bB.substr(0,bw.length+1)===bw+"-":false},POS:function(bz,bw,bx,bA){var e=bw[2],by=bk.setFilters[e];if(by){return by(bz,bx,bw,bA)}}}};var bj=bk.match.POS,be=function(bw,e){return"\\"+(e-0+1)};for(var bg in bk.match){bk.match[bg]=new RegExp(bk.match[bg].source+(/(?![^\[]*\])(?![^\(]*\))/.source));bk.leftMatch[bg]=new RegExp(/(^(?:.|\r|\n)*?)/.source+bk.match[bg].source.replace(/\\(\d+)/g,be))}var bl=function(bw,e){bw=Array.prototype.slice.call(bw,0);if(e){e.push.apply(e,bw);return e}return bw};try{Array.prototype.slice.call(al.documentElement.childNodes,0)[0].nodeType}catch(bu){bl=function(bz,by){var bx=0,bw=by||[];if(br.call(bz)==="[object Array]"){Array.prototype.push.apply(bw,bz)}else{if(typeof bz.length==="number"){for(var e=bz.length;bx<e;bx++){bw.push(bz[bx])}}else{for(;bz[bx];bx++){bw.push(bz[bx])}}}return bw}}var bq,bm;if(al.documentElement.compareDocumentPosition){bq=function(bw,e){if(bw===e){bi=true;return 0}if(!bw.compareDocumentPosition||!e.compareDocumentPosition){return bw.compareDocumentPosition?-1:1}return bw.compareDocumentPosition(e)&4?-1:1}}else{bq=function(bD,bC){var bA,bw,bx=[],e=[],bz=bD.parentNode,bB=bC.parentNode,bE=bz;if(bD===bC){bi=true;return 0}else{if(bz===bB){return bm(bD,bC)}else{if(!bz){return -1}else{if(!bB){return 1}}}}while(bE){bx.unshift(bE);bE=bE.parentNode}bE=bB;while(bE){e.unshift(bE);bE=bE.parentNode}bA=bx.length;bw=e.length;for(var by=0;by<bA&&by<bw;by++){if(bx[by]!==e[by]){return bm(bx[by],e[by])}}return by===bA?bm(bD,e[by],-1):bm(bx[by],bC,1)};bm=function(bw,e,bx){if(bw===e){return bx}var by=bw.nextSibling;while(by){if(by===e){return -1}by=by.nextSibling}return 1}}bf.getText=function(e){var bw="",by;for(var bx=0;e[bx];bx++){by=e[bx];if(by.nodeType===3||by.nodeType===4){bw+=by.nodeValue}else{if(by.nodeType!==8){bw+=bf.getText(by.childNodes)}}}return bw};(function(){var bw=al.createElement("div"),bx="script"+(new Date()).getTime(),e=al.documentElement;bw.innerHTML="<a name='"+bx+"'/>";e.insertBefore(bw,e.firstChild);if(al.getElementById(bx)){bk.find.ID=function(bz,bA,bB){if(typeof bA.getElementById!=="undefined"&&!bB){var by=bA.getElementById(bz[1]);return by?by.id===bz[1]||typeof by.getAttributeNode!=="undefined"&&by.getAttributeNode("id").nodeValue===bz[1]?[by]:H:[]}};bk.filter.ID=function(bA,by){var bz=typeof bA.getAttributeNode!=="undefined"&&bA.getAttributeNode("id");return bA.nodeType===1&&bz&&bz.nodeValue===by}}e.removeChild(bw);e=bw=null})();(function(){var e=al.createElement("div");e.appendChild(al.createComment(""));if(e.getElementsByTagName("*").length>0){bk.find.TAG=function(bw,bA){var bz=bA.getElementsByTagName(bw[1]);if(bw[1]==="*"){var by=[];for(var bx=0;bz[bx];bx++){if(bz[bx].nodeType===1){by.push(bz[bx])}}bz=by}return bz}}e.innerHTML="<a href='#'></a>";if(e.firstChild&&typeof e.firstChild.getAttribute!=="undefined"&&e.firstChild.getAttribute("href")!=="#"){bk.attrHandle.href=function(bw){return bw.getAttribute("href",2)}}e=null})();if(al.querySelectorAll){(function(){var e=bf,by=al.createElement("div"),bx="__sizzle__";by.innerHTML="<p class='TEST'></p>";if(by.querySelectorAll&&by.querySelectorAll(".TEST").length===0){return}bf=function(bJ,bA,bE,bI){bA=bA||al;if(!bI&&!bf.isXML(bA)){var bH=/^(\w+$)|^\.([\w\-]+$)|^#([\w\-]+$)/.exec(bJ);if(bH&&(bA.nodeType===1||bA.nodeType===9)){if(bH[1]){return bl(bA.getElementsByTagName(bJ),bE)}else{if(bH[2]&&bk.find.CLASS&&bA.getElementsByClassName){return bl(bA.getElementsByClassName(bH[2]),bE)}}}if(bA.nodeType===9){if(bJ==="body"&&bA.body){return bl([bA.body],bE)}else{if(bH&&bH[3]){var bD=bA.getElementById(bH[3]);if(bD&&bD.parentNode){if(bD.id===bH[3]){return bl([bD],bE)}}else{return bl([],bE)}}}try{return bl(bA.querySelectorAll(bJ),bE)}catch(bF){}}else{if(bA.nodeType===1&&bA.nodeName.toLowerCase()!=="object"){var bB=bA,bC=bA.getAttribute("id"),bz=bC||bx,bL=bA.parentNode,bK=/^\s*[+~]/.test(bJ);if(!bC){bA.setAttribute("id",bz)}else{bz=bz.replace(/'/g,"\\$&")}if(bK&&bL){bA=bA.parentNode}try{if(!bK||bL){return bl(bA.querySelectorAll("[id='"+bz+"'] "+bJ),bE)}}catch(bG){}finally{if(!bC){bB.removeAttribute("id")}}}}}return e(bJ,bA,bE,bI)};for(var bw in e){bf[bw]=e[bw]}by=null})()}(function(){var e=al.documentElement,bx=e.matchesSelector||e.mozMatchesSelector||e.webkitMatchesSelector||e.msMatchesSelector,bw=false;try{bx.call(al.documentElement,"[test!='']:sizzle")}catch(by){bw=true}if(bx){bf.matchesSelector=function(bz,bB){bB=bB.replace(/\=\s*([^'"\]]*)\s*\]/g,"='$1']");if(!bf.isXML(bz)){try{if(bw||!bk.match.PSEUDO.test(bB)&&!/!=/.test(bB)){return bx.call(bz,bB)}}catch(bA){}}return bf(bB,null,null,[bz]).length>0}}})();(function(){var e=al.createElement("div");e.innerHTML="<div class='test e'></div><div class='test'></div>";if(!e.getElementsByClassName||e.getElementsByClassName("e").length===0){return}e.lastChild.className="e";if(e.getElementsByClassName("e").length===1){return}bk.order.splice(1,0,"CLASS");bk.find.CLASS=function(bw,bx,by){if(typeof bx.getElementsByClassName!=="undefined"&&!by){return bx.getElementsByClassName(bw[1])}};e=null})();function bd(bw,bB,bA,bE,bC,bD){for(var by=0,bx=bE.length;by<bx;by++){var e=bE[by];if(e){var bz=false;e=e[bw];while(e){if(e.sizcache===bA){bz=bE[e.sizset];break}if(e.nodeType===1&&!bD){e.sizcache=bA;e.sizset=by}if(e.nodeName.toLowerCase()===bB){bz=e;break}e=e[bw]}bE[by]=bz}}}function bt(bw,bB,bA,bE,bC,bD){for(var by=0,bx=bE.length;by<bx;by++){var e=bE[by];if(e){var bz=false;e=e[bw];while(e){if(e.sizcache===bA){bz=bE[e.sizset];break}if(e.nodeType===1){if(!bD){e.sizcache=bA;e.sizset=by}if(typeof bB!=="string"){if(e===bB){bz=true;break}}else{if(bf.filter(bB,[e]).length>0){bz=e;break}}}e=e[bw]}bE[by]=bz}}}if(al.documentElement.contains){bf.contains=function(bw,e){return bw!==e&&(bw.contains?bw.contains(e):true)}}else{if(al.documentElement.compareDocumentPosition){bf.contains=function(bw,e){return !!(bw.compareDocumentPosition(e)&16)}}else{bf.contains=function(){return false}}}bf.isXML=function(e){var bw=(e?e.ownerDocument||e:0).documentElement;return bw?bw.nodeName!=="HTML":false};var bs=function(e,bC){var bA,by=[],bz="",bx=bC.nodeType?[bC]:bC;while((bA=bk.match.PSEUDO.exec(e))){bz+=bA[0];e=e.replace(bk.match.PSEUDO,"")}e=bk.relative[e]?e+"*":e;for(var bB=0,bw=bx.length;bB<bw;bB++){bf(e,bx[bB],by)}return bf.filter(bz,by)};a.find=bf;a.expr=bf.selectors;a.expr[":"]=a.expr.filters;a.unique=bf.uniqueSort;a.text=bf.getText;a.isXMLDoc=bf.isXML;a.contains=bf.contains})();var W=/Until$/,ai=/^(?:parents|prevUntil|prevAll)/,aW=/,/,a9=/^.[^:#\[\.,]*$/,M=Array.prototype.slice,F=a.expr.match.POS,ao={children:true,contents:true,next:true,prev:true};a.fn.extend({find:function(e){var be=this.pushStack("","find",e),bh=0;for(var bf=0,bd=this.length;bf<bd;bf++){bh=be.length;a.find(e,this[bf],be);if(bf>0){for(var bi=bh;bi<be.length;bi++){for(var bg=0;bg<bh;bg++){if(be[bg]===be[bi]){be.splice(bi--,1);break}}}}}return be},has:function(bd){var e=a(bd);return this.filter(function(){for(var bf=0,be=e.length;bf<be;bf++){if(a.contains(this,e[bf])){return true}}})},not:function(e){return this.pushStack(av(this,e,false),"not",e)},filter:function(e){return this.pushStack(av(this,e,true),"filter",e)},is:function(e){return !!e&&a.filter(e,this).length>0},closest:function(bm,bd){var bj=[],bg,be,bl=this[0];if(a.isArray(bm)){var bi,bf,bh={},e=1;if(bl&&bm.length){for(bg=0,be=bm.length;bg<be;bg++){bf=bm[bg];if(!bh[bf]){bh[bf]=a.expr.match.POS.test(bf)?a(bf,bd||this.context):bf}}while(bl&&bl.ownerDocument&&bl!==bd){for(bf in bh){bi=bh[bf];if(bi.jquery?bi.index(bl)>-1:a(bl).is(bi)){bj.push({selector:bf,elem:bl,level:e})}}bl=bl.parentNode;e++}}return bj}var bk=F.test(bm)?a(bm,bd||this.context):null;for(bg=0,be=this.length;bg<be;bg++){bl=this[bg];while(bl){if(bk?bk.index(bl)>-1:a.find.matchesSelector(bl,bm)){bj.push(bl);break}else{bl=bl.parentNode;if(!bl||!bl.ownerDocument||bl===bd){break}}}}bj=bj.length>1?a.unique(bj):bj;return this.pushStack(bj,"closest",bm)},index:function(e){if(!e||typeof e==="string"){return a.inArray(this[0],e?a(e):this.parent().children())}return a.inArray(e.jquery?e[0]:e,this)},add:function(e,bd){var bf=typeof e==="string"?a(e,bd):a.makeArray(e),be=a.merge(this.get(),bf);return this.pushStack(B(bf[0])||B(be[0])?be:a.unique(be))},andSelf:function(){return this.add(this.prevObject)}});function B(e){return !e||!e.parentNode||e.parentNode.nodeType===11}a.each({parent:function(bd){var e=bd.parentNode;return e&&e.nodeType!==11?e:null},parents:function(e){return a.dir(e,"parentNode")},parentsUntil:function(bd,e,be){return a.dir(bd,"parentNode",be)},next:function(e){return a.nth(e,2,"nextSibling")},prev:function(e){return a.nth(e,2,"previousSibling")},nextAll:function(e){return a.dir(e,"nextSibling")},prevAll:function(e){return a.dir(e,"previousSibling")},nextUntil:function(bd,e,be){return a.dir(bd,"nextSibling",be)},prevUntil:function(bd,e,be){return a.dir(bd,"previousSibling",be)},siblings:function(e){return a.sibling(e.parentNode.firstChild,e)},children:function(e){return a.sibling(e.firstChild)},contents:function(e){return a.nodeName(e,"iframe")?e.contentDocument||e.contentWindow.document:a.makeArray(e.childNodes)}},function(e,bd){a.fn[e]=function(bh,be){var bg=a.map(this,bd,bh),bf=M.call(arguments);if(!W.test(e)){be=bh}if(be&&typeof be==="string"){bg=a.filter(be,bg)}bg=this.length>1&&!ao[e]?a.unique(bg):bg;if((this.length>1||aW.test(be))&&ai.test(e)){bg=bg.reverse()}return this.pushStack(bg,e,bf.join(","))}});a.extend({filter:function(be,e,bd){if(bd){be=":not("+be+")"}return e.length===1?a.find.matchesSelector(e[0],be)?[e[0]]:[]:a.find.matches(be,e)},dir:function(be,bd,bg){var e=[],bf=be[bd];while(bf&&bf.nodeType!==9&&(bg===H||bf.nodeType!==1||!a(bf).is(bg))){if(bf.nodeType===1){e.push(bf)}bf=bf[bd]}return e},nth:function(bg,e,be,bf){e=e||1;var bd=0;for(;bg;bg=bg[be]){if(bg.nodeType===1&&++bd===e){break}}return bg},sibling:function(be,bd){var e=[];for(;be;be=be.nextSibling){if(be.nodeType===1&&be!==bd){e.push(be)}}return e}});function av(bf,be,e){if(a.isFunction(be)){return a.grep(bf,function(bh,bg){var bi=!!be.call(bh,bg,bh);return bi===e})}else{if(be.nodeType){return a.grep(bf,function(bh,bg){return(bh===be)===e})}else{if(typeof be==="string"){var bd=a.grep(bf,function(bg){return bg.nodeType===1});if(a9.test(be)){return a.filter(be,bd,!e)}else{be=a.filter(be,bd)}}}}return a.grep(bf,function(bh,bg){return(a.inArray(bh,be)>=0)===e})}var ab=/ jQuery\d+="(?:\d+|null)"/g,aj=/^\s+/,O=/<(?!area|br|col|embed|hr|img|input|link|meta|param)(([\w:]+)[^>]*)\/>/ig,c=/<([\w:]+)/,v=/<tbody/i,T=/<|&#?\w+;/,L=/<(?:script|object|embed|option|style)/i,m=/checked\s*(?:[^=]|=\s*.checked.)/i,an={option:[1,"<select multiple='multiple'>","</select>"],legend:[1,"<fieldset>","</fieldset>"],thead:[1,"<table>","</table>"],tr:[2,"<table><tbody>","</tbody></table>"],td:[3,"<table><tbody><tr>","</tr></tbody></table>"],col:[2,"<table><tbody></tbody><colgroup>","</colgroup></table>"],area:[1,"<map>","</map>"],_default:[0,"",""]};an.optgroup=an.option;an.tbody=an.tfoot=an.colgroup=an.caption=an.thead;an.th=an.td;if(!a.support.htmlSerialize){an._default=[1,"div<div>","</div>"]}a.fn.extend({text:function(e){if(a.isFunction(e)){return this.each(function(be){var bd=a(this);bd.text(e.call(this,be,bd.text()))})}if(typeof e!=="object"&&e!==H){return this.empty().append((this[0]&&this[0].ownerDocument||al).createTextNode(e))}return a.text(this)},wrapAll:function(e){if(a.isFunction(e)){return this.each(function(be){a(this).wrapAll(e.call(this,be))})}if(this[0]){var bd=a(e,this[0].ownerDocument).eq(0).clone(true);if(this[0].parentNode){bd.insertBefore(this[0])}bd.map(function(){var be=this;while(be.firstChild&&be.firstChild.nodeType===1){be=be.firstChild}return be}).append(this)}return this},wrapInner:function(e){if(a.isFunction(e)){return this.each(function(bd){a(this).wrapInner(e.call(this,bd))})}return this.each(function(){var bd=a(this),be=bd.contents();if(be.length){be.wrapAll(e)}else{bd.append(e)}})},wrap:function(e){return this.each(function(){a(this).wrapAll(e)})},unwrap:function(){return this.parent().each(function(){if(!a.nodeName(this,"body")){a(this).replaceWith(this.childNodes)}}).end()},append:function(){return this.domManip(arguments,true,function(e){if(this.nodeType===1){this.appendChild(e)}})},prepend:function(){return this.domManip(arguments,true,function(e){if(this.nodeType===1){this.insertBefore(e,this.firstChild)}})},before:function(){if(this[0]&&this[0].parentNode){return this.domManip(arguments,false,function(bd){this.parentNode.insertBefore(bd,this)})}else{if(arguments.length){var e=a(arguments[0]);e.push.apply(e,this.toArray());return this.pushStack(e,"before",arguments)}}},after:function(){if(this[0]&&this[0].parentNode){return this.domManip(arguments,false,function(bd){this.parentNode.insertBefore(bd,this.nextSibling)})}else{if(arguments.length){var e=this.pushStack(this,"after",arguments);e.push.apply(e,a(arguments[0]).toArray());return e}}},remove:function(e,bf){for(var bd=0,be;(be=this[bd])!=null;bd++){if(!e||a.filter(e,[be]).length){if(!bf&&be.nodeType===1){a.cleanData(be.getElementsByTagName("*"));a.cleanData([be])}if(be.parentNode){be.parentNode.removeChild(be)}}}return this},empty:function(){for(var e=0,bd;(bd=this[e])!=null;e++){if(bd.nodeType===1){a.cleanData(bd.getElementsByTagName("*"))}while(bd.firstChild){bd.removeChild(bd.firstChild)}}return this},clone:function(bd,e){bd=bd==null?false:bd;e=e==null?bd:e;return this.map(function(){return a.clone(this,bd,e)})},html:function(bf){if(bf===H){return this[0]&&this[0].nodeType===1?this[0].innerHTML.replace(ab,""):null}else{if(typeof bf==="string"&&!L.test(bf)&&(a.support.leadingWhitespace||!aj.test(bf))&&!an[(c.exec(bf)||["",""])[1].toLowerCase()]){bf=bf.replace(O,"<$1></$2>");try{for(var be=0,bd=this.length;be<bd;be++){if(this[be].nodeType===1){a.cleanData(this[be].getElementsByTagName("*"));this[be].innerHTML=bf}}}catch(bg){this.empty().append(bf)}}else{if(a.isFunction(bf)){this.each(function(bh){var e=a(this);e.html(bf.call(this,bh,e.html()))})}else{this.empty().append(bf)}}}return this},replaceWith:function(e){if(this[0]&&this[0].parentNode){if(a.isFunction(e)){return this.each(function(bf){var be=a(this),bd=be.html();be.replaceWith(e.call(this,bf,bd))})}if(typeof e!=="string"){e=a(e).detach()}return this.each(function(){var be=this.nextSibling,bd=this.parentNode;a(this).remove();if(be){a(be).before(e)}else{a(bd).append(e)}})}else{return this.pushStack(a(a.isFunction(e)?e():e),"replaceWith",e)}},detach:function(e){return this.remove(e,true)},domManip:function(bj,bn,bm){var bf,bg,bi,bl,bk=bj[0],bd=[];if(!a.support.checkClone&&arguments.length===3&&typeof bk==="string"&&m.test(bk)){return this.each(function(){a(this).domManip(bj,bn,bm,true)})}if(a.isFunction(bk)){return this.each(function(bp){var bo=a(this);bj[0]=bk.call(this,bp,bn?bo.html():H);bo.domManip(bj,bn,bm)})}if(this[0]){bl=bk&&bk.parentNode;if(a.support.parentNode&&bl&&bl.nodeType===11&&bl.childNodes.length===this.length){bf={fragment:bl}}else{bf=a.buildFragment(bj,this,bd)}bi=bf.fragment;if(bi.childNodes.length===1){bg=bi=bi.firstChild}else{bg=bi.firstChild}if(bg){bn=bn&&a.nodeName(bg,"tr");for(var be=0,e=this.length,bh=e-1;be<e;be++){bm.call(bn?aX(this[be],bg):this[be],bf.cacheable||(e>1&&be<bh)?a.clone(bi,true,true):bi)}}if(bd.length){a.each(bd,a8)}}return this}});function aX(e,bd){return a.nodeName(e,"table")?(e.getElementsByTagName("tbody")[0]||e.appendChild(e.ownerDocument.createElement("tbody"))):e}function s(e,bj){if(bj.nodeType!==1||!a.hasData(e)){return}var bi=a.expando,bf=a.data(e),bg=a.data(bj,bf);if((bf=bf[bi])){var bk=bf.events;bg=bg[bi]=a.extend({},bf);if(bk){delete bg.handle;bg.events={};for(var bh in bk){for(var be=0,bd=bk[bh].length;be<bd;be++){a.event.add(bj,bh+(bk[bh][be].namespace?".":"")+bk[bh][be].namespace,bk[bh][be],bk[bh][be].data)}}}}}function ac(bd,e){if(e.nodeType!==1){return}var be=e.nodeName.toLowerCase();e.clearAttributes();e.mergeAttributes(bd);if(be==="object"){e.outerHTML=bd.outerHTML}else{if(be==="input"&&(bd.type==="checkbox"||bd.type==="radio")){if(bd.checked){e.defaultChecked=e.checked=bd.checked}if(e.value!==bd.value){e.value=bd.value}}else{if(be==="option"){e.selected=bd.defaultSelected}else{if(be==="input"||be==="textarea"){e.defaultValue=bd.defaultValue}}}}e.removeAttribute(a.expando)}a.buildFragment=function(bh,bf,bd){var bg,e,be,bi=(bf&&bf[0]?bf[0].ownerDocument||bf[0]:al);if(bh.length===1&&typeof bh[0]==="string"&&bh[0].length<512&&bi===al&&bh[0].charAt(0)==="<"&&!L.test(bh[0])&&(a.support.checkClone||!m.test(bh[0]))){e=true;be=a.fragments[bh[0]];if(be){if(be!==1){bg=be}}}if(!bg){bg=bi.createDocumentFragment();a.clean(bh,bi,bg,bd)}if(e){a.fragments[bh[0]]=be?bg:1}return{fragment:bg,cacheable:e}};a.fragments={};a.each({appendTo:"append",prependTo:"prepend",insertBefore:"before",insertAfter:"after",replaceAll:"replaceWith"},function(e,bd){a.fn[e]=function(be){var bh=[],bk=a(be),bj=this.length===1&&this[0].parentNode;if(bj&&bj.nodeType===11&&bj.childNodes.length===1&&bk.length===1){bk[bd](this[0]);return this}else{for(var bi=0,bf=bk.length;bi<bf;bi++){var bg=(bi>0?this.clone(true):this).get();a(bk[bi])[bd](bg);bh=bh.concat(bg)}return this.pushStack(bh,e,bk.selector)}}});function a1(e){if("getElementsByTagName" in e){return e.getElementsByTagName("*")}else{if("querySelectorAll" in e){return e.querySelectorAll("*")}else{return[]}}}a.extend({clone:function(bg,bi,be){var bh=bg.cloneNode(true),e,bd,bf;if((!a.support.noCloneEvent||!a.support.noCloneChecked)&&(bg.nodeType===1||bg.nodeType===11)&&!a.isXMLDoc(bg)){ac(bg,bh);e=a1(bg);bd=a1(bh);for(bf=0;e[bf];++bf){ac(e[bf],bd[bf])}}if(bi){s(bg,bh);if(be){e=a1(bg);bd=a1(bh);for(bf=0;e[bf];++bf){s(e[bf],bd[bf])}}}return bh},clean:function(be,bg,bn,bi){bg=bg||al;if(typeof bg.createElement==="undefined"){bg=bg.ownerDocument||bg[0]&&bg[0].ownerDocument||al}var bo=[];for(var bm=0,bh;(bh=be[bm])!=null;bm++){if(typeof bh==="number"){bh+=""}if(!bh){continue}if(typeof bh==="string"&&!T.test(bh)){bh=bg.createTextNode(bh)}else{if(typeof bh==="string"){bh=bh.replace(O,"<$1></$2>");var bp=(c.exec(bh)||["",""])[1].toLowerCase(),bf=an[bp]||an._default,bl=bf[0],bd=bg.createElement("div");bd.innerHTML=bf[1]+bh+bf[2];while(bl--){bd=bd.lastChild}if(!a.support.tbody){var e=v.test(bh),bk=bp==="table"&&!e?bd.firstChild&&bd.firstChild.childNodes:bf[1]==="<table>"&&!e?bd.childNodes:[];for(var bj=bk.length-1;bj>=0;--bj){if(a.nodeName(bk[bj],"tbody")&&!bk[bj].childNodes.length){bk[bj].parentNode.removeChild(bk[bj])}}}if(!a.support.leadingWhitespace&&aj.test(bh)){bd.insertBefore(bg.createTextNode(aj.exec(bh)[0]),bd.firstChild)}bh=bd.childNodes}}if(bh.nodeType){bo.push(bh)}else{bo=a.merge(bo,bh)}}if(bn){for(bm=0;bo[bm];bm++){if(bi&&a.nodeName(bo[bm],"script")&&(!bo[bm].type||bo[bm].type.toLowerCase()==="text/javascript")){bi.push(bo[bm].parentNode?bo[bm].parentNode.removeChild(bo[bm]):bo[bm])}else{if(bo[bm].nodeType===1){bo.splice.apply(bo,[bm+1,0].concat(a.makeArray(bo[bm].getElementsByTagName("script"))))}bn.appendChild(bo[bm])}}}return bo},cleanData:function(bd){var bg,be,e=a.cache,bl=a.expando,bj=a.event.special,bi=a.support.deleteExpando;for(var bh=0,bf;(bf=bd[bh])!=null;bh++){if(bf.nodeName&&a.noData[bf.nodeName.toLowerCase()]){continue}be=bf[a.expando];if(be){bg=e[be]&&e[be][bl];if(bg&&bg.events){for(var bk in bg.events){if(bj[bk]){a.event.remove(bf,bk)}else{a.removeEvent(bf,bk,bg.handle)}}if(bg.handle){bg.handle.elem=null}}if(bi){delete bf[a.expando]}else{if(bf.removeAttribute){bf.removeAttribute(a.expando)}}delete e[be]}}}});function a8(e,bd){if(bd.src){a.ajax({url:bd.src,async:false,dataType:"script"})}else{a.globalEval(bd.text||bd.textContent||bd.innerHTML||"")}if(bd.parentNode){bd.parentNode.removeChild(bd)}}var ae=/alpha\([^)]*\)/i,ak=/opacity=([^)]*)/,aM=/-([a-z])/ig,y=/([A-Z])/g,aZ=/^-?\d+(?:px)?$/i,a7=/^-?\d/,aV={position:"absolute",visibility:"hidden",display:"block"},ag=["Left","Right"],aR=["Top","Bottom"],U,ay,aL,l=function(e,bd){return bd.toUpperCase()};a.fn.css=function(e,bd){if(arguments.length===2&&bd===H){return this}return a.access(this,e,bd,true,function(bf,be,bg){return bg!==H?a.style(bf,be,bg):a.css(bf,be)})};a.extend({cssHooks:{opacity:{get:function(be,bd){if(bd){var e=U(be,"opacity","opacity");return e===""?"1":e}else{return be.style.opacity}}}},cssNumber:{zIndex:true,fontWeight:true,opacity:true,zoom:true,lineHeight:true},cssProps:{"float":a.support.cssFloat?"cssFloat":"styleFloat"},style:function(bf,be,bk,bg){if(!bf||bf.nodeType===3||bf.nodeType===8||!bf.style){return}var bj,bh=a.camelCase(be),bd=bf.style,bl=a.cssHooks[bh];be=a.cssProps[bh]||bh;if(bk!==H){if(typeof bk==="number"&&isNaN(bk)||bk==null){return}if(typeof bk==="number"&&!a.cssNumber[bh]){bk+="px"}if(!bl||!("set" in bl)||(bk=bl.set(bf,bk))!==H){try{bd[be]=bk}catch(bi){}}}else{if(bl&&"get" in bl&&(bj=bl.get(bf,false,bg))!==H){return bj}return bd[be]}},css:function(bh,bg,bd){var bf,be=a.camelCase(bg),e=a.cssHooks[be];bg=a.cssProps[be]||be;if(e&&"get" in e&&(bf=e.get(bh,true,bd))!==H){return bf}else{if(U){return U(bh,bg,be)}}},swap:function(bf,be,bg){var e={};for(var bd in be){e[bd]=bf.style[bd];bf.style[bd]=be[bd]}bg.call(bf);for(bd in be){bf.style[bd]=e[bd]}},camelCase:function(e){return e.replace(aM,l)}});a.curCSS=a.css;a.each(["height","width"],function(bd,e){a.cssHooks[e]={get:function(bg,bf,be){var bh;if(bf){if(bg.offsetWidth!==0){bh=o(bg,e,be)}else{a.swap(bg,aV,function(){bh=o(bg,e,be)})}if(bh<=0){bh=U(bg,e,e);if(bh==="0px"&&aL){bh=aL(bg,e,e)}if(bh!=null){return bh===""||bh==="auto"?"0px":bh}}if(bh<0||bh==null){bh=bg.style[e];return bh===""||bh==="auto"?"0px":bh}return typeof bh==="string"?bh:bh+"px"}},set:function(be,bf){if(aZ.test(bf)){bf=parseFloat(bf);if(bf>=0){return bf+"px"}}else{return bf}}}});if(!a.support.opacity){a.cssHooks.opacity={get:function(bd,e){return ak.test((e&&bd.currentStyle?bd.currentStyle.filter:bd.style.filter)||"")?(parseFloat(RegExp.$1)/100)+"":e?"1":""},set:function(bf,bg){var be=bf.style;be.zoom=1;var e=a.isNaN(bg)?"":"alpha(opacity="+bg*100+")",bd=be.filter||"";be.filter=ae.test(bd)?bd.replace(ae,e):be.filter+" "+e}}}if(al.defaultView&&al.defaultView.getComputedStyle){ay=function(bh,e,bf){var be,bg,bd;bf=bf.replace(y,"-$1").toLowerCase();if(!(bg=bh.ownerDocument.defaultView)){return H}if((bd=bg.getComputedStyle(bh,null))){be=bd.getPropertyValue(bf);if(be===""&&!a.contains(bh.ownerDocument.documentElement,bh)){be=a.style(bh,bf)}}return be}}if(al.documentElement.currentStyle){aL=function(bg,be){var bh,bd=bg.currentStyle&&bg.currentStyle[be],e=bg.runtimeStyle&&bg.runtimeStyle[be],bf=bg.style;if(!aZ.test(bd)&&a7.test(bd)){bh=bf.left;if(e){bg.runtimeStyle.left=bg.currentStyle.left}bf.left=be==="fontSize"?"1em":(bd||0);bd=bf.pixelLeft+"px";bf.left=bh;if(e){bg.runtimeStyle.left=e}}return bd===""?"auto":bd}}U=ay||aL;function o(be,bd,e){var bg=bd==="width"?ag:aR,bf=bd==="width"?be.offsetWidth:be.offsetHeight;if(e==="border"){return bf}a.each(bg,function(){if(!e){bf-=parseFloat(a.css(be,"padding"+this))||0}if(e==="margin"){bf+=parseFloat(a.css(be,"margin"+this))||0}else{bf-=parseFloat(a.css(be,"border"+this+"Width"))||0}});return bf}if(a.expr&&a.expr.filters){a.expr.filters.hidden=function(be){var bd=be.offsetWidth,e=be.offsetHeight;return(bd===0&&e===0)||(!a.support.reliableHiddenOffsets&&(be.style.display||a.css(be,"display"))==="none")};a.expr.filters.visible=function(e){return !a.expr.filters.hidden(e)}}var i=/%20/g,ah=/\[\]$/,bc=/\r?\n/g,ba=/#.*$/,ar=/^(.*?):[ \t]*([^\r\n]*)\r?$/mg,aO=/^(?:color|date|datetime|email|hidden|month|number|password|range|search|tel|text|time|url|week)$/i,aB=/(?:^file|^widget|\-extension):$/,aD=/^(?:GET|HEAD)$/,b=/^\/\//,I=/\?/,aU=/<script\b[^<]*(?:(?!<\/script>)<[^<]*)*<\/script>/gi,p=/^(?:select|textarea)/i,g=/\s+/,bb=/([?&])_=[^&]*/,R=/(^|\-)([a-z])/g,aJ=function(bd,e,be){return e+be.toUpperCase()},G=/^([\w\+\.\-]+:)\/\/([^\/?#:]*)(?::(\d+))?/,z=a.fn.load,V={},q={},au,r;try{au=al.location.href}catch(am){au=al.createElement("a");au.href="";au=au.href}r=G.exec(au.toLowerCase());function d(e){return function(bg,bi){if(typeof bg!=="string"){bi=bg;bg="*"}if(a.isFunction(bi)){var bf=bg.toLowerCase().split(g),be=0,bh=bf.length,bd,bj,bk;for(;be<bh;be++){bd=bf[be];bk=/^\+/.test(bd);if(bk){bd=bd.substr(1)||"*"}bj=e[bd]=e[bd]||[];bj[bk?"unshift":"push"](bi)}}}}function aI(bd,bm,bh,bl,bj,bf){bj=bj||bm.dataTypes[0];bf=bf||{};bf[bj]=true;var bi=bd[bj],be=0,e=bi?bi.length:0,bg=(bd===V),bk;for(;be<e&&(bg||!bk);be++){bk=bi[be](bm,bh,bl);if(typeof bk==="string"){if(!bg||bf[bk]){bk=H}else{bm.dataTypes.unshift(bk);bk=aI(bd,bm,bh,bl,bk,bf)}}}if((bg||!bk)&&!bf["*"]){bk=aI(bd,bm,bh,bl,"*",bf)}return bk}a.fn.extend({load:function(be,bh,bi){if(typeof be!=="string"&&z){return z.apply(this,arguments)}else{if(!this.length){return this}}var bg=be.indexOf(" ");if(bg>=0){var e=be.slice(bg,be.length);be=be.slice(0,bg)}var bf="GET";if(bh){if(a.isFunction(bh)){bi=bh;bh=H}else{if(typeof bh==="object"){bh=a.param(bh,a.ajaxSettings.traditional);bf="POST"}}}var bd=this;a.ajax({url:be,type:bf,dataType:"html",data:bh,complete:function(bk,bj,bl){bl=bk.responseText;if(bk.isResolved()){bk.done(function(bm){bl=bm});bd.html(e?a("<div>").append(bl.replace(aU,"")).find(e):bl)}if(bi){bd.each(bi,[bl,bj,bk])}}});return this},serialize:function(){return a.param(this.serializeArray())},serializeArray:function(){return this.map(function(){return this.elements?a.makeArray(this.elements):this}).filter(function(){return this.name&&!this.disabled&&(this.checked||p.test(this.nodeName)||aO.test(this.type))}).map(function(e,bd){var be=a(this).val();return be==null?null:a.isArray(be)?a.map(be,function(bg,bf){return{name:bd.name,value:bg.replace(bc,"\r\n")}}):{name:bd.name,value:be.replace(bc,"\r\n")}}).get()}});a.each("ajaxStart ajaxStop ajaxComplete ajaxError ajaxSuccess ajaxSend".split(" "),function(e,bd){a.fn[bd]=function(be){return this.bind(bd,be)}});a.each(["get","post"],function(e,bd){a[bd]=function(be,bg,bh,bf){if(a.isFunction(bg)){bf=bf||bh;bh=bg;bg=H}return a.ajax({type:bd,url:be,data:bg,success:bh,dataType:bf})}});a.extend({getScript:function(e,bd){return a.get(e,H,bd,"script")},getJSON:function(e,bd,be){return a.get(e,bd,be,"json")},ajaxSetup:function(be,e){if(!e){e=be;be=a.extend(true,a.ajaxSettings,e)}else{a.extend(true,be,a.ajaxSettings,e)}for(var bd in {context:1,url:1}){if(bd in e){be[bd]=e[bd]}else{if(bd in a.ajaxSettings){be[bd]=a.ajaxSettings[bd]}}}return be},ajaxSettings:{url:au,isLocal:aB.test(r[1]),global:true,type:"GET",contentType:"application/x-www-form-urlencoded",processData:true,async:true,accepts:{xml:"application/xml, text/xml",html:"text/html",text:"text/plain",json:"application/json, text/javascript","*":"*/*"},contents:{xml:/xml/,html:/html/,json:/json/},responseFields:{xml:"responseXML",text:"responseText"},converters:{"* text":aY.String,"text html":true,"text json":a.parseJSON,"text xml":a.parseXML}},ajaxPrefilter:d(V),ajaxTransport:d(q),ajax:function(bh,bf){if(typeof bh==="object"){bf=bh;bh=H}bf=bf||{};var bl=a.ajaxSetup({},bf),bz=bl.context||bl,bo=bz!==bl&&(bz.nodeType||bz instanceof a)?a(bz):a.event,by=a.Deferred(),bv=a._Deferred(),bj=bl.statusCode||{},bk,bp={},bx,bg,bt,bm,bq,bi=0,be,bs,br={readyState:0,setRequestHeader:function(e,bA){if(!bi){bp[e.toLowerCase().replace(R,aJ)]=bA}return this},getAllResponseHeaders:function(){return bi===2?bx:null},getResponseHeader:function(bA){var e;if(bi===2){if(!bg){bg={};while((e=ar.exec(bx))){bg[e[1].toLowerCase()]=e[2]}}e=bg[bA.toLowerCase()]}return e===H?null:e},overrideMimeType:function(e){if(!bi){bl.mimeType=e}return this},abort:function(e){e=e||"abort";if(bt){bt.abort(e)}bn(0,e);return this}};function bn(bF,bD,bG,bC){if(bi===2){return}bi=2;if(bm){clearTimeout(bm)}bt=H;bx=bC||"";br.readyState=bF?4:0;var bA,bK,bJ,bE=bG?a4(bl,br,bG):H,bB,bI;if(bF>=200&&bF<300||bF===304){if(bl.ifModified){if((bB=br.getResponseHeader("Last-Modified"))){a.lastModified[bk]=bB}if((bI=br.getResponseHeader("Etag"))){a.etag[bk]=bI}}if(bF===304){bD="notmodified";bA=true}else{try{bK=D(bl,bE);bD="success";bA=true}catch(bH){bD="parsererror";bJ=bH}}}else{bJ=bD;if(!bD||bF){bD="error";if(bF<0){bF=0}}}br.status=bF;br.statusText=bD;if(bA){by.resolveWith(bz,[bK,bD,br])}else{by.rejectWith(bz,[br,bD,bJ])}br.statusCode(bj);bj=H;if(be){bo.trigger("ajax"+(bA?"Success":"Error"),[br,bl,bA?bK:bJ])}bv.resolveWith(bz,[br,bD]);if(be){bo.trigger("ajaxComplete",[br,bl]);if(!(--a.active)){a.event.trigger("ajaxStop")}}}by.promise(br);br.success=br.done;br.error=br.fail;br.complete=bv.done;br.statusCode=function(bA){if(bA){var e;if(bi<2){for(e in bA){bj[e]=[bj[e],bA[e]]}}else{e=bA[br.status];br.then(e,e)}}return this};bl.url=((bh||bl.url)+"").replace(ba,"").replace(b,r[1]+"//");bl.dataTypes=a.trim(bl.dataType||"*").toLowerCase().split(g);if(!bl.crossDomain){bq=G.exec(bl.url.toLowerCase());bl.crossDomain=!!(bq&&(bq[1]!=r[1]||bq[2]!=r[2]||(bq[3]||(bq[1]==="http:"?80:443))!=(r[3]||(r[1]==="http:"?80:443))))}if(bl.data&&bl.processData&&typeof bl.data!=="string"){bl.data=a.param(bl.data,bl.traditional)}aI(V,bl,bf,br);if(bi===2){return false}be=bl.global;bl.type=bl.type.toUpperCase();bl.hasContent=!aD.test(bl.type);if(be&&a.active++===0){a.event.trigger("ajaxStart")}if(!bl.hasContent){if(bl.data){bl.url+=(I.test(bl.url)?"&":"?")+bl.data}bk=bl.url;if(bl.cache===false){var bd=a.now(),bw=bl.url.replace(bb,"$1_="+bd);bl.url=bw+((bw===bl.url)?(I.test(bl.url)?"&":"?")+"_="+bd:"")}}if(bl.data&&bl.hasContent&&bl.contentType!==false||bf.contentType){bp["Content-Type"]=bl.contentType}if(bl.ifModified){bk=bk||bl.url;if(a.lastModified[bk]){bp["If-Modified-Since"]=a.lastModified[bk]}if(a.etag[bk]){bp["If-None-Match"]=a.etag[bk]}}bp.Accept=bl.dataTypes[0]&&bl.accepts[bl.dataTypes[0]]?bl.accepts[bl.dataTypes[0]]+(bl.dataTypes[0]!=="*"?", */*; q=0.01":""):bl.accepts["*"];for(bs in bl.headers){br.setRequestHeader(bs,bl.headers[bs])}if(bl.beforeSend&&(bl.beforeSend.call(bz,br,bl)===false||bi===2)){br.abort();return false}for(bs in {success:1,error:1,complete:1}){br[bs](bl[bs])}bt=aI(q,bl,bf,br);if(!bt){bn(-1,"No Transport")}else{br.readyState=1;if(be){bo.trigger("ajaxSend",[br,bl])}if(bl.async&&bl.timeout>0){bm=setTimeout(function(){br.abort("timeout")},bl.timeout)}try{bi=1;bt.send(bp,bn)}catch(bu){if(status<2){bn(-1,bu)}else{a.error(bu)}}}return br},param:function(e,be){var bd=[],bg=function(bh,bi){bi=a.isFunction(bi)?bi():bi;bd[bd.length]=encodeURIComponent(bh)+"="+encodeURIComponent(bi)};if(be===H){be=a.ajaxSettings.traditional}if(a.isArray(e)||(e.jquery&&!a.isPlainObject(e))){a.each(e,function(){bg(this.name,this.value)})}else{for(var bf in e){u(bf,e[bf],be,bg)}}return bd.join("&").replace(i,"+")}});function u(be,bg,bd,bf){if(a.isArray(bg)&&bg.length){a.each(bg,function(bi,bh){if(bd||ah.test(be)){bf(be,bh)}else{u(be+"["+(typeof bh==="object"||a.isArray(bh)?bi:"")+"]",bh,bd,bf)}})}else{if(!bd&&bg!=null&&typeof bg==="object"){if(a.isArray(bg)||a.isEmptyObject(bg)){bf(be,"")}else{for(var e in bg){u(be+"["+e+"]",bg[e],bd,bf)}}}else{bf(be,bg)}}}a.extend({active:0,lastModified:{},etag:{}});function a4(bl,bk,bh){var bd=bl.contents,bj=bl.dataTypes,be=bl.responseFields,bg,bi,bf,e;for(bi in be){if(bi in bh){bk[be[bi]]=bh[bi]}}while(bj[0]==="*"){bj.shift();if(bg===H){bg=bl.mimeType||bk.getResponseHeader("content-type")}}if(bg){for(bi in bd){if(bd[bi]&&bd[bi].test(bg)){bj.unshift(bi);break}}}if(bj[0] in bh){bf=bj[0]}else{for(bi in bh){if(!bj[0]||bl.converters[bi+" "+bj[0]]){bf=bi;break}if(!e){e=bi}}bf=bf||e}if(bf){if(bf!==bj[0]){bj.unshift(bf)}return bh[bf]}}function D(bp,bh){if(bp.dataFilter){bh=bp.dataFilter(bh,bp.dataType)}var bl=bp.dataTypes,bo={},bi,bm,be=bl.length,bj,bk=bl[0],bf,bg,bn,bd,e;for(bi=1;bi<be;bi++){if(bi===1){for(bm in bp.converters){if(typeof bm==="string"){bo[bm.toLowerCase()]=bp.converters[bm]}}}bf=bk;bk=bl[bi];if(bk==="*"){bk=bf}else{if(bf!=="*"&&bf!==bk){bg=bf+" "+bk;bn=bo[bg]||bo["* "+bk];if(!bn){e=H;for(bd in bo){bj=bd.split(" ");if(bj[0]===bf||bj[0]==="*"){e=bo[bj[1]+" "+bk];if(e){bd=bo[bd];if(bd===true){bn=e}else{if(e===true){bn=bd}}break}}}}if(!(bn||e)){a.error("No conversion from "+bg.replace(" "," to "))}if(bn!==true){bh=bn?bn(bh):e(bd(bh))}}}}return bh}var aq=a.now(),t=/(\=)\?(&|$)|()\?\?()/i;a.ajaxSetup({jsonp:"callback",jsonpCallback:function(){return a.expando+"_"+(aq++)}});a.ajaxPrefilter("json jsonp",function(bm,bi,bl){var bk=(typeof bm.data==="string");if(bm.dataTypes[0]==="jsonp"||bi.jsonpCallback||bi.jsonp!=null||bm.jsonp!==false&&(t.test(bm.url)||bk&&t.test(bm.data))){var bj,be=bm.jsonpCallback=a.isFunction(bm.jsonpCallback)?bm.jsonpCallback():bm.jsonpCallback,bh=aY[be],e=bm.url,bg=bm.data,bd="$1"+be+"$2",bf=function(){aY[be]=bh;if(bj&&a.isFunction(bh)){aY[be](bj[0])}};if(bm.jsonp!==false){e=e.replace(t,bd);if(bm.url===e){if(bk){bg=bg.replace(t,bd)}if(bm.data===bg){e+=(/\?/.test(e)?"&":"?")+bm.jsonp+"="+be}}}bm.url=e;bm.data=bg;aY[be]=function(bn){bj=[bn]};bl.then(bf,bf);bm.converters["script json"]=function(){if(!bj){a.error(be+" was not called")}return bj[0]};bm.dataTypes[0]="json";return"script"}});a.ajaxSetup({accepts:{script:"text/javascript, application/javascript, application/ecmascript, application/x-ecmascript"},contents:{script:/javascript|ecmascript/},converters:{"text script":function(e){a.globalEval(e);return e}}});a.ajaxPrefilter("script",function(e){if(e.cache===H){e.cache=false}if(e.crossDomain){e.type="GET";e.global=false}});a.ajaxTransport("script",function(be){if(be.crossDomain){var e,bd=al.head||al.getElementsByTagName("head")[0]||al.documentElement;return{send:function(bf,bg){e=al.createElement("script");e.async="async";if(be.scriptCharset){e.charset=be.scriptCharset}e.src=be.url;e.onload=e.onreadystatechange=function(bi,bh){if(!e.readyState||/loaded|complete/.test(e.readyState)){e.onload=e.onreadystatechange=null;if(bd&&e.parentNode){bd.removeChild(e)}e=H;if(!bh){bg(200,"success")}}};bd.insertBefore(e,bd.firstChild)},abort:function(){if(e){e.onload(0,1)}}}}});var x=a.now(),J,at;function A(){a(aY).unload(function(){for(var e in J){J[e](0,1)}})}function aA(){try{return new aY.XMLHttpRequest()}catch(bd){}}function ad(){try{return new aY.ActiveXObject("Microsoft.XMLHTTP")}catch(bd){}}a.ajaxSettings.xhr=aY.ActiveXObject?function(){return !this.isLocal&&aA()||ad()}:aA;at=a.ajaxSettings.xhr();a.support.ajax=!!at;a.support.cors=at&&("withCredentials" in at);at=H;if(a.support.ajax){a.ajaxTransport(function(e){if(!e.crossDomain||a.support.cors){var bd;return{send:function(bj,be){var bi=e.xhr(),bh,bg;if(e.username){bi.open(e.type,e.url,e.async,e.username,e.password)}else{bi.open(e.type,e.url,e.async)}if(e.xhrFields){for(bg in e.xhrFields){bi[bg]=e.xhrFields[bg]}}if(e.mimeType&&bi.overrideMimeType){bi.overrideMimeType(e.mimeType)}if(!(e.crossDomain&&!e.hasContent)&&!bj["X-Requested-With"]){bj["X-Requested-With"]="XMLHttpRequest"}try{for(bg in bj){bi.setRequestHeader(bg,bj[bg])}}catch(bf){}bi.send((e.hasContent&&e.data)||null);bd=function(bs,bm){var bn,bl,bk,bq,bp;try{if(bd&&(bm||bi.readyState===4)){bd=H;if(bh){bi.onreadystatechange=a.noop;delete J[bh]}if(bm){if(bi.readyState!==4){bi.abort()}}else{bn=bi.status;bk=bi.getAllResponseHeaders();bq={};bp=bi.responseXML;if(bp&&bp.documentElement){bq.xml=bp}bq.text=bi.responseText;try{bl=bi.statusText}catch(br){bl=""}if(!bn&&e.isLocal&&!e.crossDomain){bn=bq.text?200:404}else{if(bn===1223){bn=204}}}}}catch(bo){if(!bm){be(-1,bo)}}if(bq){be(bn,bl,bq,bk)}};if(!e.async||bi.readyState===4){bd()}else{if(!J){J={};A()}bh=x++;bi.onreadystatechange=J[bh]=bd}},abort:function(){if(bd){bd(0,1)}}}}})}var N={},ap=/^(?:toggle|show|hide)$/,aF=/^([+\-]=)?([\d+.\-]+)([a-z%]*)$/i,aS,ax=[["height","marginTop","marginBottom","paddingTop","paddingBottom"],["width","marginLeft","marginRight","paddingLeft","paddingRight"],["opacity"]];a.fn.extend({show:function(bf,bi,bh){var be,bg;if(bf||bf===0){return this.animate(aQ("show",3),bf,bi,bh)}else{for(var bd=0,e=this.length;bd<e;bd++){be=this[bd];bg=be.style.display;if(!a._data(be,"olddisplay")&&bg==="none"){bg=be.style.display=""}if(bg===""&&a.css(be,"display")==="none"){a._data(be,"olddisplay",w(be.nodeName))}}for(bd=0;bd<e;bd++){be=this[bd];bg=be.style.display;if(bg===""||bg==="none"){be.style.display=a._data(be,"olddisplay")||""}}return this}},hide:function(be,bh,bg){if(be||be===0){return this.animate(aQ("hide",3),be,bh,bg)}else{for(var bd=0,e=this.length;bd<e;bd++){var bf=a.css(this[bd],"display");if(bf!=="none"&&!a._data(this[bd],"olddisplay")){a._data(this[bd],"olddisplay",bf)}}for(bd=0;bd<e;bd++){this[bd].style.display="none"}return this}},_toggle:a.fn.toggle,toggle:function(be,bd,bf){var e=typeof be==="boolean";if(a.isFunction(be)&&a.isFunction(bd)){this._toggle.apply(this,arguments)}else{if(be==null||e){this.each(function(){var bg=e?be:a(this).is(":hidden");a(this)[bg?"show":"hide"]()})}else{this.animate(aQ("toggle",3),be,bd,bf)}}return this},fadeTo:function(e,bf,be,bd){return this.filter(":hidden").css("opacity",0).show().end().animate({opacity:bf},e,be,bd)},animate:function(bg,bd,bf,be){var e=a.speed(bd,bf,be);if(a.isEmptyObject(bg)){return this.each(e.complete)}return this[e.queue===false?"each":"queue"](function(){var bj=a.extend({},e),bn,bk=this.nodeType===1,bl=bk&&a(this).is(":hidden"),bh=this;for(bn in bg){var bi=a.camelCase(bn);if(bn!==bi){bg[bi]=bg[bn];delete bg[bn];bn=bi}if(bg[bn]==="hide"&&bl||bg[bn]==="show"&&!bl){return bj.complete.call(this)}if(bk&&(bn==="height"||bn==="width")){bj.overflow=[this.style.overflow,this.style.overflowX,this.style.overflowY];if(a.css(this,"display")==="inline"&&a.css(this,"float")==="none"){if(!a.support.inlineBlockNeedsLayout){this.style.display="inline-block"}else{var bm=w(this.nodeName);if(bm==="inline"){this.style.display="inline-block"}else{this.style.display="inline";this.style.zoom=1}}}}if(a.isArray(bg[bn])){(bj.specialEasing=bj.specialEasing||{})[bn]=bg[bn][1];bg[bn]=bg[bn][0]}}if(bj.overflow!=null){this.style.overflow="hidden"}bj.curAnim=a.extend({},bg);a.each(bg,function(bp,bt){var bs=new a.fx(bh,bj,bp);if(ap.test(bt)){bs[bt==="toggle"?bl?"show":"hide":bt](bg)}else{var br=aF.exec(bt),bu=bs.cur();if(br){var bo=parseFloat(br[2]),bq=br[3]||(a.cssNumber[bp]?"":"px");if(bq!=="px"){a.style(bh,bp,(bo||1)+bq);bu=((bo||1)/bs.cur())*bu;a.style(bh,bp,bu+bq)}if(br[1]){bo=((br[1]==="-="?-1:1)*bo)+bu}bs.custom(bu,bo,bq)}else{bs.custom(bu,bt,"")}}});return true})},stop:function(bd,e){var be=a.timers;if(bd){this.queue([])}this.each(function(){for(var bf=be.length-1;bf>=0;bf--){if(be[bf].elem===this){if(e){be[bf](true)}be.splice(bf,1)}}});if(!e){this.dequeue()}return this}});function aQ(bd,e){var be={};a.each(ax.concat.apply([],ax.slice(0,e)),function(){be[this]=bd});return be}a.each({slideDown:aQ("show",1),slideUp:aQ("hide",1),slideToggle:aQ("toggle",1),fadeIn:{opacity:"show"},fadeOut:{opacity:"hide"},fadeToggle:{opacity:"toggle"}},function(e,bd){a.fn[e]=function(be,bg,bf){return this.animate(bd,be,bg,bf)}});a.extend({speed:function(be,bf,bd){var e=be&&typeof be==="object"?a.extend({},be):{complete:bd||!bd&&bf||a.isFunction(be)&&be,duration:be,easing:bd&&bf||bf&&!a.isFunction(bf)&&bf};e.duration=a.fx.off?0:typeof e.duration==="number"?e.duration:e.duration in a.fx.speeds?a.fx.speeds[e.duration]:a.fx.speeds._default;e.old=e.complete;e.complete=function(){if(e.queue!==false){a(this).dequeue()}if(a.isFunction(e.old)){e.old.call(this)}};return e},easing:{linear:function(be,bf,e,bd){return e+bd*be},swing:function(be,bf,e,bd){return((-Math.cos(be*Math.PI)/2)+0.5)*bd+e}},timers:[],fx:function(bd,e,be){this.options=e;this.elem=bd;this.prop=be;if(!e.orig){e.orig={}}}});a.fx.prototype={update:function(){if(this.options.step){this.options.step.call(this.elem,this.now,this)}(a.fx.step[this.prop]||a.fx.step._default)(this)},cur:function(){if(this.elem[this.prop]!=null&&(!this.elem.style||this.elem.style[this.prop]==null)){return this.elem[this.prop]}var e,bd=a.css(this.elem,this.prop);return isNaN(e=parseFloat(bd))?!bd||bd==="auto"?0:bd:e},custom:function(bh,bg,bf){var e=this,be=a.fx;this.startTime=a.now();this.start=bh;this.end=bg;this.unit=bf||this.unit||(a.cssNumber[this.prop]?"":"px");this.now=this.start;this.pos=this.state=0;function bd(bi){return e.step(bi)}bd.elem=this.elem;if(bd()&&a.timers.push(bd)&&!aS){aS=setInterval(be.tick,be.interval)}},show:function(){this.options.orig[this.prop]=a.style(this.elem,this.prop);this.options.show=true;this.custom(this.prop==="width"||this.prop==="height"?1:0,this.cur());a(this.elem).show()},hide:function(){this.options.orig[this.prop]=a.style(this.elem,this.prop);this.options.hide=true;this.custom(this.cur(),0)},step:function(bf){var bk=a.now(),bg=true;if(bf||bk>=this.options.duration+this.startTime){this.now=this.end;this.pos=this.state=1;this.update();this.options.curAnim[this.prop]=true;for(var bh in this.options.curAnim){if(this.options.curAnim[bh]!==true){bg=false}}if(bg){if(this.options.overflow!=null&&!a.support.shrinkWrapBlocks){var be=this.elem,bl=this.options;a.each(["","X","Y"],function(bm,bn){be.style["overflow"+bn]=bl.overflow[bm]})}if(this.options.hide){a(this.elem).hide()}if(this.options.hide||this.options.show){for(var e in this.options.curAnim){a.style(this.elem,e,this.options.orig[e])}}this.options.complete.call(this.elem)}return false}else{var bd=bk-this.startTime;this.state=bd/this.options.duration;var bi=this.options.specialEasing&&this.options.specialEasing[this.prop];var bj=this.options.easing||(a.easing.swing?"swing":"linear");this.pos=a.easing[bi||bj](this.state,bd,0,1,this.options.duration);this.now=this.start+((this.end-this.start)*this.pos);this.update()}return true}};a.extend(a.fx,{tick:function(){var bd=a.timers;for(var e=0;e<bd.length;e++){if(!bd[e]()){bd.splice(e--,1)}}if(!bd.length){a.fx.stop()}},interval:13,stop:function(){clearInterval(aS);aS=null},speeds:{slow:600,fast:200,_default:400},step:{opacity:function(e){a.style(e.elem,"opacity",e.now)},_default:function(e){if(e.elem.style&&e.elem.style[e.prop]!=null){e.elem.style[e.prop]=(e.prop==="width"||e.prop==="height"?Math.max(0,e.now):e.now)+e.unit}else{e.elem[e.prop]=e.now}}}});if(a.expr&&a.expr.filters){a.expr.filters.animated=function(e){return a.grep(a.timers,function(bd){return e===bd.elem}).length}}function w(be){if(!N[be]){var e=a("<"+be+">").appendTo("body"),bd=e.css("display");e.remove();if(bd==="none"||bd===""){bd="block"}N[be]=bd}return N[be]}var S=/^t(?:able|d|h)$/i,Y=/^(?:body|html)$/i;if("getBoundingClientRect" in al.documentElement){a.fn.offset=function(bq){var bg=this[0],bj;if(bq){return this.each(function(e){a.offset.setOffset(this,bq,e)})}if(!bg||!bg.ownerDocument){return null}if(bg===bg.ownerDocument.body){return a.offset.bodyOffset(bg)}try{bj=bg.getBoundingClientRect()}catch(bn){}var bp=bg.ownerDocument,be=bp.documentElement;if(!bj||!a.contains(be,bg)){return bj?{top:bj.top,left:bj.left}:{top:0,left:0}}var bk=bp.body,bl=az(bp),bi=be.clientTop||bk.clientTop||0,bm=be.clientLeft||bk.clientLeft||0,bd=(bl.pageYOffset||a.support.boxModel&&be.scrollTop||bk.scrollTop),bh=(bl.pageXOffset||a.support.boxModel&&be.scrollLeft||bk.scrollLeft),bo=bj.top+bd-bi,bf=bj.left+bh-bm;return{top:bo,left:bf}}}else{a.fn.offset=function(bn){var bh=this[0];if(bn){return this.each(function(bo){a.offset.setOffset(this,bn,bo)})}if(!bh||!bh.ownerDocument){return null}if(bh===bh.ownerDocument.body){return a.offset.bodyOffset(bh)}a.offset.initialize();var bk,be=bh.offsetParent,bd=bh,bm=bh.ownerDocument,bf=bm.documentElement,bi=bm.body,bj=bm.defaultView,e=bj?bj.getComputedStyle(bh,null):bh.currentStyle,bl=bh.offsetTop,bg=bh.offsetLeft;while((bh=bh.parentNode)&&bh!==bi&&bh!==bf){if(a.offset.supportsFixedPosition&&e.position==="fixed"){break}bk=bj?bj.getComputedStyle(bh,null):bh.currentStyle;bl-=bh.scrollTop;bg-=bh.scrollLeft;if(bh===be){bl+=bh.offsetTop;bg+=bh.offsetLeft;if(a.offset.doesNotAddBorder&&!(a.offset.doesAddBorderForTableAndCells&&S.test(bh.nodeName))){bl+=parseFloat(bk.borderTopWidth)||0;bg+=parseFloat(bk.borderLeftWidth)||0}bd=be;be=bh.offsetParent}if(a.offset.subtractsBorderForOverflowNotVisible&&bk.overflow!=="visible"){bl+=parseFloat(bk.borderTopWidth)||0;bg+=parseFloat(bk.borderLeftWidth)||0}e=bk}if(e.position==="relative"||e.position==="static"){bl+=bi.offsetTop;bg+=bi.offsetLeft}if(a.offset.supportsFixedPosition&&e.position==="fixed"){bl+=Math.max(bf.scrollTop,bi.scrollTop);bg+=Math.max(bf.scrollLeft,bi.scrollLeft)}return{top:bl,left:bg}}}a.offset={initialize:function(){var e=al.body,bd=al.createElement("div"),bg,bi,bh,bj,be=parseFloat(a.css(e,"marginTop"))||0,bf="<div style='position:absolute;top:0;left:0;margin:0;border:5px solid #000;padding:0;width:1px;height:1px;'><div></div></div><table style='position:absolute;top:0;left:0;margin:0;border:5px solid #000;padding:0;width:1px;height:1px;' cellpadding='0' cellspacing='0'><tr><td></td></tr></table>";a.extend(bd.style,{position:"absolute",top:0,left:0,margin:0,border:0,width:"1px",height:"1px",visibility:"hidden"});bd.innerHTML=bf;e.insertBefore(bd,e.firstChild);bg=bd.firstChild;bi=bg.firstChild;bj=bg.nextSibling.firstChild.firstChild;this.doesNotAddBorder=(bi.offsetTop!==5);this.doesAddBorderForTableAndCells=(bj.offsetTop===5);bi.style.position="fixed";bi.style.top="20px";this.supportsFixedPosition=(bi.offsetTop===20||bi.offsetTop===15);bi.style.position=bi.style.top="";bg.style.overflow="hidden";bg.style.position="relative";this.subtractsBorderForOverflowNotVisible=(bi.offsetTop===-5);this.doesNotIncludeMarginInBodyOffset=(e.offsetTop!==be);e.removeChild(bd);e=bd=bg=bi=bh=bj=null;a.offset.initialize=a.noop},bodyOffset:function(e){var be=e.offsetTop,bd=e.offsetLeft;a.offset.initialize();if(a.offset.doesNotIncludeMarginInBodyOffset){be+=parseFloat(a.css(e,"marginTop"))||0;bd+=parseFloat(a.css(e,"marginLeft"))||0}return{top:be,left:bd}},setOffset:function(bf,bo,bi){var bj=a.css(bf,"position");if(bj==="static"){bf.style.position="relative"}var bh=a(bf),bd=bh.offset(),e=a.css(bf,"top"),bm=a.css(bf,"left"),bn=(bj==="absolute"&&a.inArray("auto",[e,bm])>-1),bl={},bk={},be,bg;if(bn){bk=bh.position()}be=bn?bk.top:parseInt(e,10)||0;bg=bn?bk.left:parseInt(bm,10)||0;if(a.isFunction(bo)){bo=bo.call(bf,bi,bd)}if(bo.top!=null){bl.top=(bo.top-bd.top)+be}if(bo.left!=null){bl.left=(bo.left-bd.left)+bg}if("using" in bo){bo.using.call(bf,bl)}else{bh.css(bl)}}};a.fn.extend({position:function(){if(!this[0]){return null}var be=this[0],bd=this.offsetParent(),bf=this.offset(),e=Y.test(bd[0].nodeName)?{top:0,left:0}:bd.offset();bf.top-=parseFloat(a.css(be,"marginTop"))||0;bf.left-=parseFloat(a.css(be,"marginLeft"))||0;e.top+=parseFloat(a.css(bd[0],"borderTopWidth"))||0;e.left+=parseFloat(a.css(bd[0],"borderLeftWidth"))||0;return{top:bf.top-e.top,left:bf.left-e.left}},offsetParent:function(){return this.map(function(){var e=this.offsetParent||al.body;while(e&&(!Y.test(e.nodeName)&&a.css(e,"position")==="static")){e=e.offsetParent}return e})}});a.each(["Left","Top"],function(bd,e){var be="scroll"+e;a.fn[be]=function(bh){var bf=this[0],bg;if(!bf){return null}if(bh!==H){return this.each(function(){bg=az(this);if(bg){bg.scrollTo(!bd?bh:a(bg).scrollLeft(),bd?bh:a(bg).scrollTop())}else{this[be]=bh}})}else{bg=az(bf);return bg?("pageXOffset" in bg)?bg[bd?"pageYOffset":"pageXOffset"]:a.support.boxModel&&bg.document.documentElement[be]||bg.document.body[be]:bf[be]}}});function az(e){return a.isWindow(e)?e:e.nodeType===9?e.defaultView||e.parentWindow:false}a.each(["Height","Width"],function(bd,e){var be=e.toLowerCase();a.fn["inner"+e]=function(){return this[0]?parseFloat(a.css(this[0],be,"padding")):null};a.fn["outer"+e]=function(bf){return this[0]?parseFloat(a.css(this[0],be,bf?"margin":"border")):null};a.fn[be]=function(bg){var bh=this[0];if(!bh){return bg==null?null:this}if(a.isFunction(bg)){return this.each(function(bl){var bk=a(this);bk[be](bg.call(this,bl,bk[be]()))})}if(a.isWindow(bh)){var bi=bh.document.documentElement["client"+e];return bh.document.compatMode==="CSS1Compat"&&bi||bh.document.body["client"+e]||bi}else{if(bh.nodeType===9){return Math.max(bh.documentElement["client"+e],bh.body["scroll"+e],bh.documentElement["scroll"+e],bh.body["offset"+e],bh.documentElement["offset"+e])}else{if(bg===H){var bj=a.css(bh,be),bf=parseFloat(bj);return a.isNaN(bf)?bj:bf}else{return this.css(be,typeof bg==="string"?bg:bg+"px")}}}}});aY.jQuery=aY.$=a})(window);/* Javascript plotting library for jQuery, v. 0.7.
 *
 * Released under the MIT license by IOLA, December 2007.
 *
 */
(function(b){b.color={};b.color.make=function(d,e,g,f){var c={};c.r=d||0;c.g=e||0;c.b=g||0;c.a=f!=null?f:1;c.add=function(h,j){for(var k=0;k<h.length;++k){c[h.charAt(k)]+=j}return c.normalize()};c.scale=function(h,j){for(var k=0;k<h.length;++k){c[h.charAt(k)]*=j}return c.normalize()};c.toString=function(){if(c.a>=1){return"rgb("+[c.r,c.g,c.b].join(",")+")"}else{return"rgba("+[c.r,c.g,c.b,c.a].join(",")+")"}};c.normalize=function(){function h(k,j,l){return j<k?k:(j>l?l:j)}c.r=h(0,parseInt(c.r),255);c.g=h(0,parseInt(c.g),255);c.b=h(0,parseInt(c.b),255);c.a=h(0,c.a,1);return c};c.clone=function(){return b.color.make(c.r,c.b,c.g,c.a)};return c.normalize()};b.color.extract=function(d,e){var c;do{c=d.css(e).toLowerCase();if(c!=""&&c!="transparent"){break}d=d.parent()}while(!b.nodeName(d.get(0),"body"));if(c=="rgba(0, 0, 0, 0)"){c="transparent"}return b.color.parse(c)};b.color.parse=function(c){var d,f=b.color.make;if(d=/rgb\(\s*([0-9]{1,3})\s*,\s*([0-9]{1,3})\s*,\s*([0-9]{1,3})\s*\)/.exec(c)){return f(parseInt(d[1],10),parseInt(d[2],10),parseInt(d[3],10))}if(d=/rgba\(\s*([0-9]{1,3})\s*,\s*([0-9]{1,3})\s*,\s*([0-9]{1,3})\s*,\s*([0-9]+(?:\.[0-9]+)?)\s*\)/.exec(c)){return f(parseInt(d[1],10),parseInt(d[2],10),parseInt(d[3],10),parseFloat(d[4]))}if(d=/rgb\(\s*([0-9]+(?:\.[0-9]+)?)\%\s*,\s*([0-9]+(?:\.[0-9]+)?)\%\s*,\s*([0-9]+(?:\.[0-9]+)?)\%\s*\)/.exec(c)){return f(parseFloat(d[1])*2.55,parseFloat(d[2])*2.55,parseFloat(d[3])*2.55)}if(d=/rgba\(\s*([0-9]+(?:\.[0-9]+)?)\%\s*,\s*([0-9]+(?:\.[0-9]+)?)\%\s*,\s*([0-9]+(?:\.[0-9]+)?)\%\s*,\s*([0-9]+(?:\.[0-9]+)?)\s*\)/.exec(c)){return f(parseFloat(d[1])*2.55,parseFloat(d[2])*2.55,parseFloat(d[3])*2.55,parseFloat(d[4]))}if(d=/#([a-fA-F0-9]{2})([a-fA-F0-9]{2})([a-fA-F0-9]{2})/.exec(c)){return f(parseInt(d[1],16),parseInt(d[2],16),parseInt(d[3],16))}if(d=/#([a-fA-F0-9])([a-fA-F0-9])([a-fA-F0-9])/.exec(c)){return f(parseInt(d[1]+d[1],16),parseInt(d[2]+d[2],16),parseInt(d[3]+d[3],16))}var e=b.trim(c).toLowerCase();if(e=="transparent"){return f(255,255,255,0)}else{d=a[e]||[0,0,0];return f(d[0],d[1],d[2])}};var a={aqua:[0,255,255],azure:[240,255,255],beige:[245,245,220],black:[0,0,0],blue:[0,0,255],brown:[165,42,42],cyan:[0,255,255],darkblue:[0,0,139],darkcyan:[0,139,139],darkgrey:[169,169,169],darkgreen:[0,100,0],darkkhaki:[189,183,107],darkmagenta:[139,0,139],darkolivegreen:[85,107,47],darkorange:[255,140,0],darkorchid:[153,50,204],darkred:[139,0,0],darksalmon:[233,150,122],darkviolet:[148,0,211],fuchsia:[255,0,255],gold:[255,215,0],green:[0,128,0],indigo:[75,0,130],khaki:[240,230,140],lightblue:[173,216,230],lightcyan:[224,255,255],lightgreen:[144,238,144],lightgrey:[211,211,211],lightpink:[255,182,193],lightyellow:[255,255,224],lime:[0,255,0],magenta:[255,0,255],maroon:[128,0,0],navy:[0,0,128],olive:[128,128,0],orange:[255,165,0],pink:[255,192,203],purple:[128,0,128],violet:[128,0,128],red:[255,0,0],silver:[192,192,192],white:[255,255,255],yellow:[255,255,0]}})(jQuery);(function(c){function b(av,ai,J,af){var Q=[],O={colors:["#edc240","#afd8f8","#cb4b4b","#4da74d","#9440ed"],legend:{show:true,noColumns:1,labelFormatter:null,labelBoxBorderColor:"#ccc",container:null,position:"ne",margin:5,backgroundColor:null,backgroundOpacity:0.85},xaxis:{show:null,position:"bottom",mode:null,color:null,tickColor:null,transform:null,inverseTransform:null,min:null,max:null,autoscaleMargin:null,ticks:null,tickFormatter:null,labelWidth:null,labelHeight:null,reserveSpace:null,tickLength:null,alignTicksWithAxis:null,tickDecimals:null,tickSize:null,minTickSize:null,monthNames:null,timeformat:null,twelveHourClock:false},yaxis:{autoscaleMargin:0.02,position:"left"},xaxes:[],yaxes:[],series:{points:{show:false,radius:3,lineWidth:2,fill:true,fillColor:"#ffffff",symbol:"circle"},lines:{lineWidth:2,fill:false,fillColor:null,steps:false},bars:{show:false,lineWidth:2,barWidth:1,fill:true,fillColor:null,align:"left",horizontal:false},shadowSize:3},grid:{show:true,aboveData:false,color:"#545454",backgroundColor:null,borderColor:null,tickColor:null,labelMargin:5,axisMargin:8,borderWidth:2,minBorderMargin:null,markings:null,markingsColor:"#f4f4f4",markingsLineWidth:2,clickable:false,hoverable:false,autoHighlight:true,mouseActiveRadius:10},hooks:{}},az=null,ad=null,y=null,H=null,A=null,p=[],aw=[],q={left:0,right:0,top:0,bottom:0},G=0,I=0,h=0,w=0,ak={processOptions:[],processRawData:[],processDatapoints:[],drawSeries:[],draw:[],bindEvents:[],drawOverlay:[],shutdown:[]},aq=this;aq.setData=aj;aq.setupGrid=t;aq.draw=W;aq.getPlaceholder=function(){return av};aq.getCanvas=function(){return az};aq.getPlotOffset=function(){return q};aq.width=function(){return h};aq.height=function(){return w};aq.offset=function(){var aB=y.offset();aB.left+=q.left;aB.top+=q.top;return aB};aq.getData=function(){return Q};aq.getAxes=function(){var aC={},aB;c.each(p.concat(aw),function(aD,aE){if(aE){aC[aE.direction+(aE.n!=1?aE.n:"")+"axis"]=aE}});return aC};aq.getXAxes=function(){return p};aq.getYAxes=function(){return aw};aq.c2p=C;aq.p2c=ar;aq.getOptions=function(){return O};aq.highlight=x;aq.unhighlight=T;aq.triggerRedrawOverlay=f;aq.pointOffset=function(aB){return{left:parseInt(p[aA(aB,"x")-1].p2c(+aB.x)+q.left),top:parseInt(aw[aA(aB,"y")-1].p2c(+aB.y)+q.top)}};aq.shutdown=ag;aq.resize=function(){B();g(az);g(ad)};aq.hooks=ak;F(aq);Z(J);X();aj(ai);t();W();ah();function an(aD,aB){aB=[aq].concat(aB);for(var aC=0;aC<aD.length;++aC){aD[aC].apply(this,aB)}}function F(){for(var aB=0;aB<af.length;++aB){var aC=af[aB];aC.init(aq);if(aC.options){c.extend(true,O,aC.options)}}}function Z(aC){var aB;c.extend(true,O,aC);if(O.xaxis.color==null){O.xaxis.color=O.grid.color}if(O.yaxis.color==null){O.yaxis.color=O.grid.color}if(O.xaxis.tickColor==null){O.xaxis.tickColor=O.grid.tickColor}if(O.yaxis.tickColor==null){O.yaxis.tickColor=O.grid.tickColor}if(O.grid.borderColor==null){O.grid.borderColor=O.grid.color}if(O.grid.tickColor==null){O.grid.tickColor=c.color.parse(O.grid.color).scale("a",0.22).toString()}for(aB=0;aB<Math.max(1,O.xaxes.length);++aB){O.xaxes[aB]=c.extend(true,{},O.xaxis,O.xaxes[aB])}for(aB=0;aB<Math.max(1,O.yaxes.length);++aB){O.yaxes[aB]=c.extend(true,{},O.yaxis,O.yaxes[aB])}if(O.xaxis.noTicks&&O.xaxis.ticks==null){O.xaxis.ticks=O.xaxis.noTicks}if(O.yaxis.noTicks&&O.yaxis.ticks==null){O.yaxis.ticks=O.yaxis.noTicks}if(O.x2axis){O.xaxes[1]=c.extend(true,{},O.xaxis,O.x2axis);O.xaxes[1].position="top"}if(O.y2axis){O.yaxes[1]=c.extend(true,{},O.yaxis,O.y2axis);O.yaxes[1].position="right"}if(O.grid.coloredAreas){O.grid.markings=O.grid.coloredAreas}if(O.grid.coloredAreasColor){O.grid.markingsColor=O.grid.coloredAreasColor}if(O.lines){c.extend(true,O.series.lines,O.lines)}if(O.points){c.extend(true,O.series.points,O.points)}if(O.bars){c.extend(true,O.series.bars,O.bars)}if(O.shadowSize!=null){O.series.shadowSize=O.shadowSize}for(aB=0;aB<O.xaxes.length;++aB){V(p,aB+1).options=O.xaxes[aB]}for(aB=0;aB<O.yaxes.length;++aB){V(aw,aB+1).options=O.yaxes[aB]}for(var aD in ak){if(O.hooks[aD]&&O.hooks[aD].length){ak[aD]=ak[aD].concat(O.hooks[aD])}}an(ak.processOptions,[O])}function aj(aB){Q=Y(aB);ax();z()}function Y(aE){var aC=[];for(var aB=0;aB<aE.length;++aB){var aD=c.extend(true,{},O.series);if(aE[aB].data!=null){aD.data=aE[aB].data;delete aE[aB].data;c.extend(true,aD,aE[aB]);aE[aB].data=aD.data}else{aD.data=aE[aB]}aC.push(aD)}return aC}function aA(aC,aD){var aB=aC[aD+"axis"];if(typeof aB=="object"){aB=aB.n}if(typeof aB!="number"){aB=1}return aB}function m(){return c.grep(p.concat(aw),function(aB){return aB})}function C(aE){var aC={},aB,aD;for(aB=0;aB<p.length;++aB){aD=p[aB];if(aD&&aD.used){aC["x"+aD.n]=aD.c2p(aE.left)}}for(aB=0;aB<aw.length;++aB){aD=aw[aB];if(aD&&aD.used){aC["y"+aD.n]=aD.c2p(aE.top)}}if(aC.x1!==undefined){aC.x=aC.x1}if(aC.y1!==undefined){aC.y=aC.y1}return aC}function ar(aF){var aD={},aC,aE,aB;for(aC=0;aC<p.length;++aC){aE=p[aC];if(aE&&aE.used){aB="x"+aE.n;if(aF[aB]==null&&aE.n==1){aB="x"}if(aF[aB]!=null){aD.left=aE.p2c(aF[aB]);break}}}for(aC=0;aC<aw.length;++aC){aE=aw[aC];if(aE&&aE.used){aB="y"+aE.n;if(aF[aB]==null&&aE.n==1){aB="y"}if(aF[aB]!=null){aD.top=aE.p2c(aF[aB]);break}}}return aD}function V(aC,aB){if(!aC[aB-1]){aC[aB-1]={n:aB,direction:aC==p?"x":"y",options:c.extend(true,{},aC==p?O.xaxis:O.yaxis)}}return aC[aB-1]}function ax(){var aG;var aM=Q.length,aB=[],aE=[];for(aG=0;aG<Q.length;++aG){var aJ=Q[aG].color;if(aJ!=null){--aM;if(typeof aJ=="number"){aE.push(aJ)}else{aB.push(c.color.parse(Q[aG].color))}}}for(aG=0;aG<aE.length;++aG){aM=Math.max(aM,aE[aG]+1)}var aC=[],aF=0;aG=0;while(aC.length<aM){var aI;if(O.colors.length==aG){aI=c.color.make(100,100,100)}else{aI=c.color.parse(O.colors[aG])}var aD=aF%2==1?-1:1;aI.scale("rgb",1+aD*Math.ceil(aF/2)*0.2);aC.push(aI);++aG;if(aG>=O.colors.length){aG=0;++aF}}var aH=0,aN;for(aG=0;aG<Q.length;++aG){aN=Q[aG];if(aN.color==null){aN.color=aC[aH].toString();++aH}else{if(typeof aN.color=="number"){aN.color=aC[aN.color].toString()}}if(aN.lines.show==null){var aL,aK=true;for(aL in aN){if(aN[aL]&&aN[aL].show){aK=false;break}}if(aK){aN.lines.show=true}}aN.xaxis=V(p,aA(aN,"x"));aN.yaxis=V(aw,aA(aN,"y"))}}function z(){var aO=Number.POSITIVE_INFINITY,aI=Number.NEGATIVE_INFINITY,aB=Number.MAX_VALUE,aU,aS,aR,aN,aD,aJ,aT,aP,aH,aG,aC,a0,aX,aL;function aF(a3,a2,a1){if(a2<a3.datamin&&a2!=-aB){a3.datamin=a2}if(a1>a3.datamax&&a1!=aB){a3.datamax=a1}}c.each(m(),function(a1,a2){a2.datamin=aO;a2.datamax=aI;a2.used=false});for(aU=0;aU<Q.length;++aU){aJ=Q[aU];aJ.datapoints={points:[]};an(ak.processRawData,[aJ,aJ.data,aJ.datapoints])}for(aU=0;aU<Q.length;++aU){aJ=Q[aU];var aZ=aJ.data,aW=aJ.datapoints.format;if(!aW){aW=[];aW.push({x:true,number:true,required:true});aW.push({y:true,number:true,required:true});if(aJ.bars.show||(aJ.lines.show&&aJ.lines.fill)){aW.push({y:true,number:true,required:false,defaultValue:0});if(aJ.bars.horizontal){delete aW[aW.length-1].y;aW[aW.length-1].x=true}}aJ.datapoints.format=aW}if(aJ.datapoints.pointsize!=null){continue}aJ.datapoints.pointsize=aW.length;aP=aJ.datapoints.pointsize;aT=aJ.datapoints.points;insertSteps=aJ.lines.show&&aJ.lines.steps;aJ.xaxis.used=aJ.yaxis.used=true;for(aS=aR=0;aS<aZ.length;++aS,aR+=aP){aL=aZ[aS];var aE=aL==null;if(!aE){for(aN=0;aN<aP;++aN){a0=aL[aN];aX=aW[aN];if(aX){if(aX.number&&a0!=null){a0=+a0;if(isNaN(a0)){a0=null}else{if(a0==Infinity){a0=aB}else{if(a0==-Infinity){a0=-aB}}}}if(a0==null){if(aX.required){aE=true}if(aX.defaultValue!=null){a0=aX.defaultValue}}}aT[aR+aN]=a0}}if(aE){for(aN=0;aN<aP;++aN){a0=aT[aR+aN];if(a0!=null){aX=aW[aN];if(aX.x){aF(aJ.xaxis,a0,a0)}if(aX.y){aF(aJ.yaxis,a0,a0)}}aT[aR+aN]=null}}else{if(insertSteps&&aR>0&&aT[aR-aP]!=null&&aT[aR-aP]!=aT[aR]&&aT[aR-aP+1]!=aT[aR+1]){for(aN=0;aN<aP;++aN){aT[aR+aP+aN]=aT[aR+aN]}aT[aR+1]=aT[aR-aP+1];aR+=aP}}}}for(aU=0;aU<Q.length;++aU){aJ=Q[aU];an(ak.processDatapoints,[aJ,aJ.datapoints])}for(aU=0;aU<Q.length;++aU){aJ=Q[aU];aT=aJ.datapoints.points,aP=aJ.datapoints.pointsize;var aK=aO,aQ=aO,aM=aI,aV=aI;for(aS=0;aS<aT.length;aS+=aP){if(aT[aS]==null){continue}for(aN=0;aN<aP;++aN){a0=aT[aS+aN];aX=aW[aN];if(!aX||a0==aB||a0==-aB){continue}if(aX.x){if(a0<aK){aK=a0}if(a0>aM){aM=a0}}if(aX.y){if(a0<aQ){aQ=a0}if(a0>aV){aV=a0}}}}if(aJ.bars.show){var aY=aJ.bars.align=="left"?0:-aJ.bars.barWidth/2;if(aJ.bars.horizontal){aQ+=aY;aV+=aY+aJ.bars.barWidth}else{aK+=aY;aM+=aY+aJ.bars.barWidth}}aF(aJ.xaxis,aK,aM);aF(aJ.yaxis,aQ,aV)}c.each(m(),function(a1,a2){if(a2.datamin==aO){a2.datamin=null}if(a2.datamax==aI){a2.datamax=null}})}function j(aB,aC){var aD=document.createElement("canvas");aD.className=aC;aD.width=G;aD.height=I;if(!aB){c(aD).css({position:"absolute",left:0,top:0})}c(aD).appendTo(av);if(!aD.getContext){aD=window.G_vmlCanvasManager.initElement(aD)}aD.getContext("2d").save();return aD}function B(){G=av.width();I=av.height();if(G<=0||I<=0){throw"Invalid dimensions for plot, width = "+G+", height = "+I}}function g(aC){if(aC.width!=G){aC.width=G}if(aC.height!=I){aC.height=I}var aB=aC.getContext("2d");aB.restore();aB.save()}function X(){var aC,aB=av.children("canvas.base"),aD=av.children("canvas.overlay");if(aB.length==0||aD==0){av.html("");av.css({padding:0});if(av.css("position")=="static"){av.css("position","relative")}B();az=j(true,"base");ad=j(false,"overlay");aC=false}else{az=aB.get(0);ad=aD.get(0);aC=true}H=az.getContext("2d");A=ad.getContext("2d");y=c([ad,az]);if(aC){av.data("plot").shutdown();aq.resize();A.clearRect(0,0,G,I);y.unbind();av.children().not([az,ad]).remove()}av.data("plot",aq)}function ah(){if(O.grid.hoverable){y.mousemove(aa);y.mouseleave(l)}if(O.grid.clickable){y.click(R)}an(ak.bindEvents,[y])}function ag(){if(M){clearTimeout(M)}y.unbind("mousemove",aa);y.unbind("mouseleave",l);y.unbind("click",R);an(ak.shutdown,[y])}function r(aG){function aC(aH){return aH}var aF,aB,aD=aG.options.transform||aC,aE=aG.options.inverseTransform;if(aG.direction=="x"){aF=aG.scale=h/Math.abs(aD(aG.max)-aD(aG.min));aB=Math.min(aD(aG.max),aD(aG.min))}else{aF=aG.scale=w/Math.abs(aD(aG.max)-aD(aG.min));aF=-aF;aB=Math.max(aD(aG.max),aD(aG.min))}if(aD==aC){aG.p2c=function(aH){return(aH-aB)*aF}}else{aG.p2c=function(aH){return(aD(aH)-aB)*aF}}if(!aE){aG.c2p=function(aH){return aB+aH/aF}}else{aG.c2p=function(aH){return aE(aB+aH/aF)}}}function L(aD){var aB=aD.options,aF,aJ=aD.ticks||[],aI=[],aE,aK=aB.labelWidth,aG=aB.labelHeight,aC;function aH(aM,aL){return c('<div style="position:absolute;top:-10000px;'+aL+'font-size:smaller"><div class="'+aD.direction+"Axis "+aD.direction+aD.n+'Axis">'+aM.join("")+"</div></div>").appendTo(av)}if(aD.direction=="x"){if(aK==null){aK=Math.floor(G/(aJ.length>0?aJ.length:1))}if(aG==null){aI=[];for(aF=0;aF<aJ.length;++aF){aE=aJ[aF].label;if(aE){aI.push('<div class="tickLabel" style="float:left;width:'+aK+'px">'+aE+"</div>")}}if(aI.length>0){aI.push('<div style="clear:left"></div>');aC=aH(aI,"width:10000px;");aG=aC.height();aC.remove()}}}else{if(aK==null||aG==null){for(aF=0;aF<aJ.length;++aF){aE=aJ[aF].label;if(aE){aI.push('<div class="tickLabel">'+aE+"</div>")}}if(aI.length>0){aC=aH(aI,"");if(aK==null){aK=aC.children().width()}if(aG==null){aG=aC.find("div.tickLabel").height()}aC.remove()}}}if(aK==null){aK=0}if(aG==null){aG=0}aD.labelWidth=aK;aD.labelHeight=aG}function au(aD){var aC=aD.labelWidth,aL=aD.labelHeight,aH=aD.options.position,aF=aD.options.tickLength,aG=O.grid.axisMargin,aJ=O.grid.labelMargin,aK=aD.direction=="x"?p:aw,aE;var aB=c.grep(aK,function(aN){return aN&&aN.options.position==aH&&aN.reserveSpace});if(c.inArray(aD,aB)==aB.length-1){aG=0}if(aF==null){aF="full"}var aI=c.grep(aK,function(aN){return aN&&aN.reserveSpace});var aM=c.inArray(aD,aI)==0;if(!aM&&aF=="full"){aF=5}if(!isNaN(+aF)){aJ+=+aF}if(aD.direction=="x"){aL+=aJ;if(aH=="bottom"){q.bottom+=aL+aG;aD.box={top:I-q.bottom,height:aL}}else{aD.box={top:q.top+aG,height:aL};q.top+=aL+aG}}else{aC+=aJ;if(aH=="left"){aD.box={left:q.left+aG,width:aC};q.left+=aC+aG}else{q.right+=aC+aG;aD.box={left:G-q.right,width:aC}}}aD.position=aH;aD.tickLength=aF;aD.box.padding=aJ;aD.innermost=aM}function U(aB){if(aB.direction=="x"){aB.box.left=q.left;aB.box.width=h}else{aB.box.top=q.top;aB.box.height=w}}function t(){var aC,aE=m();c.each(aE,function(aF,aG){aG.show=aG.options.show;if(aG.show==null){aG.show=aG.used}aG.reserveSpace=aG.show||aG.options.reserveSpace;n(aG)});allocatedAxes=c.grep(aE,function(aF){return aF.reserveSpace});q.left=q.right=q.top=q.bottom=0;if(O.grid.show){c.each(allocatedAxes,function(aF,aG){S(aG);P(aG);ap(aG,aG.ticks);L(aG)});for(aC=allocatedAxes.length-1;aC>=0;--aC){au(allocatedAxes[aC])}var aD=O.grid.minBorderMargin;if(aD==null){aD=0;for(aC=0;aC<Q.length;++aC){aD=Math.max(aD,Q[aC].points.radius+Q[aC].points.lineWidth/2)}}for(var aB in q){q[aB]+=O.grid.borderWidth;q[aB]=Math.max(aD,q[aB])}}h=G-q.left-q.right;w=I-q.bottom-q.top;c.each(aE,function(aF,aG){r(aG)});if(O.grid.show){c.each(allocatedAxes,function(aF,aG){U(aG)});k()}o()}function n(aE){var aF=aE.options,aD=+(aF.min!=null?aF.min:aE.datamin),aB=+(aF.max!=null?aF.max:aE.datamax),aH=aB-aD;if(aH==0){var aC=aB==0?1:0.01;if(aF.min==null){aD-=aC}if(aF.max==null||aF.min!=null){aB+=aC}}else{var aG=aF.autoscaleMargin;if(aG!=null){if(aF.min==null){aD-=aH*aG;if(aD<0&&aE.datamin!=null&&aE.datamin>=0){aD=0}}if(aF.max==null){aB+=aH*aG;if(aB>0&&aE.datamax!=null&&aE.datamax<=0){aB=0}}}}aE.min=aD;aE.max=aB}function S(aG){var aM=aG.options;var aH;if(typeof aM.ticks=="number"&&aM.ticks>0){aH=aM.ticks}else{aH=0.3*Math.sqrt(aG.direction=="x"?G:I)}var aT=(aG.max-aG.min)/aH,aO,aB,aN,aR,aS,aQ,aI;if(aM.mode=="time"){var aJ={second:1000,minute:60*1000,hour:60*60*1000,day:24*60*60*1000,month:30*24*60*60*1000,year:365.2425*24*60*60*1000};var aK=[[1,"second"],[2,"second"],[5,"second"],[10,"second"],[30,"second"],[1,"minute"],[2,"minute"],[5,"minute"],[10,"minute"],[30,"minute"],[1,"hour"],[2,"hour"],[4,"hour"],[8,"hour"],[12,"hour"],[1,"day"],[2,"day"],[3,"day"],[0.25,"month"],[0.5,"month"],[1,"month"],[2,"month"],[3,"month"],[6,"month"],[1,"year"]];var aC=0;if(aM.minTickSize!=null){if(typeof aM.tickSize=="number"){aC=aM.tickSize}else{aC=aM.minTickSize[0]*aJ[aM.minTickSize[1]]}}for(var aS=0;aS<aK.length-1;++aS){if(aT<(aK[aS][0]*aJ[aK[aS][1]]+aK[aS+1][0]*aJ[aK[aS+1][1]])/2&&aK[aS][0]*aJ[aK[aS][1]]>=aC){break}}aO=aK[aS][0];aN=aK[aS][1];if(aN=="year"){aQ=Math.pow(10,Math.floor(Math.log(aT/aJ.year)/Math.LN10));aI=(aT/aJ.year)/aQ;if(aI<1.5){aO=1}else{if(aI<3){aO=2}else{if(aI<7.5){aO=5}else{aO=10}}}aO*=aQ}aG.tickSize=aM.tickSize||[aO,aN];aB=function(aX){var a2=[],a0=aX.tickSize[0],a3=aX.tickSize[1],a1=new Date(aX.min);var aW=a0*aJ[a3];if(a3=="second"){a1.setUTCSeconds(a(a1.getUTCSeconds(),a0))}if(a3=="minute"){a1.setUTCMinutes(a(a1.getUTCMinutes(),a0))}if(a3=="hour"){a1.setUTCHours(a(a1.getUTCHours(),a0))}if(a3=="month"){a1.setUTCMonth(a(a1.getUTCMonth(),a0))}if(a3=="year"){a1.setUTCFullYear(a(a1.getUTCFullYear(),a0))}a1.setUTCMilliseconds(0);if(aW>=aJ.minute){a1.setUTCSeconds(0)}if(aW>=aJ.hour){a1.setUTCMinutes(0)}if(aW>=aJ.day){a1.setUTCHours(0)}if(aW>=aJ.day*4){a1.setUTCDate(1)}if(aW>=aJ.year){a1.setUTCMonth(0)}var a5=0,a4=Number.NaN,aY;do{aY=a4;a4=a1.getTime();a2.push(a4);if(a3=="month"){if(a0<1){a1.setUTCDate(1);var aV=a1.getTime();a1.setUTCMonth(a1.getUTCMonth()+1);var aZ=a1.getTime();a1.setTime(a4+a5*aJ.hour+(aZ-aV)*a0);a5=a1.getUTCHours();a1.setUTCHours(0)}else{a1.setUTCMonth(a1.getUTCMonth()+a0)}}else{if(a3=="year"){a1.setUTCFullYear(a1.getUTCFullYear()+a0)}else{a1.setTime(a4+aW)}}}while(a4<aX.max&&a4!=aY);return a2};aR=function(aV,aY){var a0=new Date(aV);if(aM.timeformat!=null){return c.plot.formatDate(a0,aM.timeformat,aM.monthNames)}var aW=aY.tickSize[0]*aJ[aY.tickSize[1]];var aX=aY.max-aY.min;var aZ=(aM.twelveHourClock)?" %p":"";if(aW<aJ.minute){fmt="%h:%M:%S"+aZ}else{if(aW<aJ.day){if(aX<2*aJ.day){fmt="%h:%M"+aZ}else{fmt="%b %d %h:%M"+aZ}}else{if(aW<aJ.month){fmt="%b %d"}else{if(aW<aJ.year){if(aX<aJ.year){fmt="%b"}else{fmt="%b %y"}}else{fmt="%y"}}}}return c.plot.formatDate(a0,fmt,aM.monthNames)}}else{var aU=aM.tickDecimals;var aP=-Math.floor(Math.log(aT)/Math.LN10);if(aU!=null&&aP>aU){aP=aU}aQ=Math.pow(10,-aP);aI=aT/aQ;if(aI<1.5){aO=1}else{if(aI<3){aO=2;if(aI>2.25&&(aU==null||aP+1<=aU)){aO=2.5;++aP}}else{if(aI<7.5){aO=5}else{aO=10}}}aO*=aQ;if(aM.minTickSize!=null&&aO<aM.minTickSize){aO=aM.minTickSize}aG.tickDecimals=Math.max(0,aU!=null?aU:aP);aG.tickSize=aM.tickSize||aO;aB=function(aX){var aZ=[];var a0=a(aX.min,aX.tickSize),aW=0,aV=Number.NaN,aY;do{aY=aV;aV=a0+aW*aX.tickSize;aZ.push(aV);++aW}while(aV<aX.max&&aV!=aY);return aZ};aR=function(aV,aW){return aV.toFixed(aW.tickDecimals)}}if(aM.alignTicksWithAxis!=null){var aF=(aG.direction=="x"?p:aw)[aM.alignTicksWithAxis-1];if(aF&&aF.used&&aF!=aG){var aL=aB(aG);if(aL.length>0){if(aM.min==null){aG.min=Math.min(aG.min,aL[0])}if(aM.max==null&&aL.length>1){aG.max=Math.max(aG.max,aL[aL.length-1])}}aB=function(aX){var aY=[],aV,aW;for(aW=0;aW<aF.ticks.length;++aW){aV=(aF.ticks[aW].v-aF.min)/(aF.max-aF.min);aV=aX.min+aV*(aX.max-aX.min);aY.push(aV)}return aY};if(aG.mode!="time"&&aM.tickDecimals==null){var aE=Math.max(0,-Math.floor(Math.log(aT)/Math.LN10)+1),aD=aB(aG);if(!(aD.length>1&&/\..*0$/.test((aD[1]-aD[0]).toFixed(aE)))){aG.tickDecimals=aE}}}}aG.tickGenerator=aB;if(c.isFunction(aM.tickFormatter)){aG.tickFormatter=function(aV,aW){return""+aM.tickFormatter(aV,aW)}}else{aG.tickFormatter=aR}}function P(aF){var aH=aF.options.ticks,aG=[];if(aH==null||(typeof aH=="number"&&aH>0)){aG=aF.tickGenerator(aF)}else{if(aH){if(c.isFunction(aH)){aG=aH({min:aF.min,max:aF.max})}else{aG=aH}}}var aE,aB;aF.ticks=[];for(aE=0;aE<aG.length;++aE){var aC=null;var aD=aG[aE];if(typeof aD=="object"){aB=+aD[0];if(aD.length>1){aC=aD[1]}}else{aB=+aD}if(aC==null){aC=aF.tickFormatter(aB,aF)}if(!isNaN(aB)){aF.ticks.push({v:aB,label:aC})}}}function ap(aB,aC){if(aB.options.autoscaleMargin&&aC.length>0){if(aB.options.min==null){aB.min=Math.min(aB.min,aC[0].v)}if(aB.options.max==null&&aC.length>1){aB.max=Math.max(aB.max,aC[aC.length-1].v)}}}function W(){H.clearRect(0,0,G,I);var aC=O.grid;if(aC.show&&aC.backgroundColor){N()}if(aC.show&&!aC.aboveData){ac()}for(var aB=0;aB<Q.length;++aB){an(ak.drawSeries,[H,Q[aB]]);d(Q[aB])}an(ak.draw,[H]);if(aC.show&&aC.aboveData){ac()}}function D(aB,aI){var aE,aH,aG,aD,aF=m();for(i=0;i<aF.length;++i){aE=aF[i];if(aE.direction==aI){aD=aI+aE.n+"axis";if(!aB[aD]&&aE.n==1){aD=aI+"axis"}if(aB[aD]){aH=aB[aD].from;aG=aB[aD].to;break}}}if(!aB[aD]){aE=aI=="x"?p[0]:aw[0];aH=aB[aI+"1"];aG=aB[aI+"2"]}if(aH!=null&&aG!=null&&aH>aG){var aC=aH;aH=aG;aG=aC}return{from:aH,to:aG,axis:aE}}function N(){H.save();H.translate(q.left,q.top);H.fillStyle=am(O.grid.backgroundColor,w,0,"rgba(255, 255, 255, 0)");H.fillRect(0,0,h,w);H.restore()}function ac(){var aF;H.save();H.translate(q.left,q.top);var aH=O.grid.markings;if(aH){if(c.isFunction(aH)){var aK=aq.getAxes();aK.xmin=aK.xaxis.min;aK.xmax=aK.xaxis.max;aK.ymin=aK.yaxis.min;aK.ymax=aK.yaxis.max;aH=aH(aK)}for(aF=0;aF<aH.length;++aF){var aD=aH[aF],aC=D(aD,"x"),aI=D(aD,"y");if(aC.from==null){aC.from=aC.axis.min}if(aC.to==null){aC.to=aC.axis.max}if(aI.from==null){aI.from=aI.axis.min}if(aI.to==null){aI.to=aI.axis.max}if(aC.to<aC.axis.min||aC.from>aC.axis.max||aI.to<aI.axis.min||aI.from>aI.axis.max){continue}aC.from=Math.max(aC.from,aC.axis.min);aC.to=Math.min(aC.to,aC.axis.max);aI.from=Math.max(aI.from,aI.axis.min);aI.to=Math.min(aI.to,aI.axis.max);if(aC.from==aC.to&&aI.from==aI.to){continue}aC.from=aC.axis.p2c(aC.from);aC.to=aC.axis.p2c(aC.to);aI.from=aI.axis.p2c(aI.from);aI.to=aI.axis.p2c(aI.to);if(aC.from==aC.to||aI.from==aI.to){H.beginPath();H.strokeStyle=aD.color||O.grid.markingsColor;H.lineWidth=aD.lineWidth||O.grid.markingsLineWidth;H.moveTo(aC.from,aI.from);H.lineTo(aC.to,aI.to);H.stroke()}else{H.fillStyle=aD.color||O.grid.markingsColor;H.fillRect(aC.from,aI.to,aC.to-aC.from,aI.from-aI.to)}}}var aK=m(),aM=O.grid.borderWidth;for(var aE=0;aE<aK.length;++aE){var aB=aK[aE],aG=aB.box,aQ=aB.tickLength,aN,aL,aP,aJ;if(!aB.show||aB.ticks.length==0){continue}H.strokeStyle=aB.options.tickColor||c.color.parse(aB.options.color).scale("a",0.22).toString();H.lineWidth=1;if(aB.direction=="x"){aN=0;if(aQ=="full"){aL=(aB.position=="top"?0:w)}else{aL=aG.top-q.top+(aB.position=="top"?aG.height:0)}}else{aL=0;if(aQ=="full"){aN=(aB.position=="left"?0:h)}else{aN=aG.left-q.left+(aB.position=="left"?aG.width:0)}}if(!aB.innermost){H.beginPath();aP=aJ=0;if(aB.direction=="x"){aP=h}else{aJ=w}if(H.lineWidth==1){aN=Math.floor(aN)+0.5;aL=Math.floor(aL)+0.5}H.moveTo(aN,aL);H.lineTo(aN+aP,aL+aJ);H.stroke()}H.beginPath();for(aF=0;aF<aB.ticks.length;++aF){var aO=aB.ticks[aF].v;aP=aJ=0;if(aO<aB.min||aO>aB.max||(aQ=="full"&&aM>0&&(aO==aB.min||aO==aB.max))){continue}if(aB.direction=="x"){aN=aB.p2c(aO);aJ=aQ=="full"?-w:aQ;if(aB.position=="top"){aJ=-aJ}}else{aL=aB.p2c(aO);aP=aQ=="full"?-h:aQ;if(aB.position=="left"){aP=-aP}}if(H.lineWidth==1){if(aB.direction=="x"){aN=Math.floor(aN)+0.5}else{aL=Math.floor(aL)+0.5}}H.moveTo(aN,aL);H.lineTo(aN+aP,aL+aJ)}H.stroke()}if(aM){H.lineWidth=aM;H.strokeStyle=O.grid.borderColor;H.strokeRect(-aM/2,-aM/2,h+aM,w+aM)}H.restore()}function k(){av.find(".tickLabels").remove();var aG=['<div class="tickLabels" style="font-size:smaller">'];var aJ=m();for(var aD=0;aD<aJ.length;++aD){var aC=aJ[aD],aF=aC.box;if(!aC.show){continue}aG.push('<div class="'+aC.direction+"Axis "+aC.direction+aC.n+'Axis" style="color:'+aC.options.color+'">');for(var aE=0;aE<aC.ticks.length;++aE){var aH=aC.ticks[aE];if(!aH.label||aH.v<aC.min||aH.v>aC.max){continue}var aK={},aI;if(aC.direction=="x"){aI="center";aK.left=Math.round(q.left+aC.p2c(aH.v)-aC.labelWidth/2);if(aC.position=="bottom"){aK.top=aF.top+aF.padding}else{aK.bottom=I-(aF.top+aF.height-aF.padding)}}else{aK.top=Math.round(q.top+aC.p2c(aH.v)-aC.labelHeight/2);if(aC.position=="left"){aK.right=G-(aF.left+aF.width-aF.padding);aI="right"}else{aK.left=aF.left+aF.padding;aI="left"}}aK.width=aC.labelWidth;var aB=["position:absolute","text-align:"+aI];for(var aL in aK){aB.push(aL+":"+aK[aL]+"px")}aG.push('<div class="tickLabel" style="'+aB.join(";")+'">'+aH.label+"</div>")}aG.push("</div>")}aG.push("</div>");av.append(aG.join(""))}function d(aB){if(aB.lines.show){at(aB)}if(aB.bars.show){e(aB)}if(aB.points.show){ao(aB)}}function at(aE){function aD(aP,aQ,aI,aU,aT){var aV=aP.points,aJ=aP.pointsize,aN=null,aM=null;H.beginPath();for(var aO=aJ;aO<aV.length;aO+=aJ){var aL=aV[aO-aJ],aS=aV[aO-aJ+1],aK=aV[aO],aR=aV[aO+1];if(aL==null||aK==null){continue}if(aS<=aR&&aS<aT.min){if(aR<aT.min){continue}aL=(aT.min-aS)/(aR-aS)*(aK-aL)+aL;aS=aT.min}else{if(aR<=aS&&aR<aT.min){if(aS<aT.min){continue}aK=(aT.min-aS)/(aR-aS)*(aK-aL)+aL;aR=aT.min}}if(aS>=aR&&aS>aT.max){if(aR>aT.max){continue}aL=(aT.max-aS)/(aR-aS)*(aK-aL)+aL;aS=aT.max}else{if(aR>=aS&&aR>aT.max){if(aS>aT.max){continue}aK=(aT.max-aS)/(aR-aS)*(aK-aL)+aL;aR=aT.max}}if(aL<=aK&&aL<aU.min){if(aK<aU.min){continue}aS=(aU.min-aL)/(aK-aL)*(aR-aS)+aS;aL=aU.min}else{if(aK<=aL&&aK<aU.min){if(aL<aU.min){continue}aR=(aU.min-aL)/(aK-aL)*(aR-aS)+aS;aK=aU.min}}if(aL>=aK&&aL>aU.max){if(aK>aU.max){continue}aS=(aU.max-aL)/(aK-aL)*(aR-aS)+aS;aL=aU.max}else{if(aK>=aL&&aK>aU.max){if(aL>aU.max){continue}aR=(aU.max-aL)/(aK-aL)*(aR-aS)+aS;aK=aU.max}}if(aL!=aN||aS!=aM){H.moveTo(aU.p2c(aL)+aQ,aT.p2c(aS)+aI)}aN=aK;aM=aR;H.lineTo(aU.p2c(aK)+aQ,aT.p2c(aR)+aI)}H.stroke()}function aF(aI,aQ,aP){var aW=aI.points,aV=aI.pointsize,aN=Math.min(Math.max(0,aP.min),aP.max),aX=0,aU,aT=false,aM=1,aL=0,aR=0;while(true){if(aV>0&&aX>aW.length+aV){break}aX+=aV;var aZ=aW[aX-aV],aK=aW[aX-aV+aM],aY=aW[aX],aJ=aW[aX+aM];if(aT){if(aV>0&&aZ!=null&&aY==null){aR=aX;aV=-aV;aM=2;continue}if(aV<0&&aX==aL+aV){H.fill();aT=false;aV=-aV;aM=1;aX=aL=aR+aV;continue}}if(aZ==null||aY==null){continue}if(aZ<=aY&&aZ<aQ.min){if(aY<aQ.min){continue}aK=(aQ.min-aZ)/(aY-aZ)*(aJ-aK)+aK;aZ=aQ.min}else{if(aY<=aZ&&aY<aQ.min){if(aZ<aQ.min){continue}aJ=(aQ.min-aZ)/(aY-aZ)*(aJ-aK)+aK;aY=aQ.min}}if(aZ>=aY&&aZ>aQ.max){if(aY>aQ.max){continue}aK=(aQ.max-aZ)/(aY-aZ)*(aJ-aK)+aK;aZ=aQ.max}else{if(aY>=aZ&&aY>aQ.max){if(aZ>aQ.max){continue}aJ=(aQ.max-aZ)/(aY-aZ)*(aJ-aK)+aK;aY=aQ.max}}if(!aT){H.beginPath();H.moveTo(aQ.p2c(aZ),aP.p2c(aN));aT=true}if(aK>=aP.max&&aJ>=aP.max){H.lineTo(aQ.p2c(aZ),aP.p2c(aP.max));H.lineTo(aQ.p2c(aY),aP.p2c(aP.max));continue}else{if(aK<=aP.min&&aJ<=aP.min){H.lineTo(aQ.p2c(aZ),aP.p2c(aP.min));H.lineTo(aQ.p2c(aY),aP.p2c(aP.min));continue}}var aO=aZ,aS=aY;if(aK<=aJ&&aK<aP.min&&aJ>=aP.min){aZ=(aP.min-aK)/(aJ-aK)*(aY-aZ)+aZ;aK=aP.min}else{if(aJ<=aK&&aJ<aP.min&&aK>=aP.min){aY=(aP.min-aK)/(aJ-aK)*(aY-aZ)+aZ;aJ=aP.min}}if(aK>=aJ&&aK>aP.max&&aJ<=aP.max){aZ=(aP.max-aK)/(aJ-aK)*(aY-aZ)+aZ;aK=aP.max}else{if(aJ>=aK&&aJ>aP.max&&aK<=aP.max){aY=(aP.max-aK)/(aJ-aK)*(aY-aZ)+aZ;aJ=aP.max}}if(aZ!=aO){H.lineTo(aQ.p2c(aO),aP.p2c(aK))}H.lineTo(aQ.p2c(aZ),aP.p2c(aK));H.lineTo(aQ.p2c(aY),aP.p2c(aJ));if(aY!=aS){H.lineTo(aQ.p2c(aY),aP.p2c(aJ));H.lineTo(aQ.p2c(aS),aP.p2c(aJ))}}}H.save();H.translate(q.left,q.top);H.lineJoin="round";var aG=aE.lines.lineWidth,aB=aE.shadowSize;if(aG>0&&aB>0){H.lineWidth=aB;H.strokeStyle="rgba(0,0,0,0.1)";var aH=Math.PI/18;aD(aE.datapoints,Math.sin(aH)*(aG/2+aB/2),Math.cos(aH)*(aG/2+aB/2),aE.xaxis,aE.yaxis);H.lineWidth=aB/2;aD(aE.datapoints,Math.sin(aH)*(aG/2+aB/4),Math.cos(aH)*(aG/2+aB/4),aE.xaxis,aE.yaxis)}H.lineWidth=aG;H.strokeStyle=aE.color;var aC=ae(aE.lines,aE.color,0,w);if(aC){H.fillStyle=aC;aF(aE.datapoints,aE.xaxis,aE.yaxis)}if(aG>0){aD(aE.datapoints,0,0,aE.xaxis,aE.yaxis)}H.restore()}function ao(aE){function aH(aN,aM,aU,aK,aS,aT,aQ,aJ){var aR=aN.points,aI=aN.pointsize;for(var aL=0;aL<aR.length;aL+=aI){var aP=aR[aL],aO=aR[aL+1];if(aP==null||aP<aT.min||aP>aT.max||aO<aQ.min||aO>aQ.max){continue}H.beginPath();aP=aT.p2c(aP);aO=aQ.p2c(aO)+aK;if(aJ=="circle"){H.arc(aP,aO,aM,0,aS?Math.PI:Math.PI*2,false)}else{aJ(H,aP,aO,aM,aS)}H.closePath();if(aU){H.fillStyle=aU;H.fill()}H.stroke()}}H.save();H.translate(q.left,q.top);var aG=aE.points.lineWidth,aC=aE.shadowSize,aB=aE.points.radius,aF=aE.points.symbol;if(aG>0&&aC>0){var aD=aC/2;H.lineWidth=aD;H.strokeStyle="rgba(0,0,0,0.1)";aH(aE.datapoints,aB,null,aD+aD/2,true,aE.xaxis,aE.yaxis,aF);H.strokeStyle="rgba(0,0,0,0.2)";aH(aE.datapoints,aB,null,aD/2,true,aE.xaxis,aE.yaxis,aF)}H.lineWidth=aG;H.strokeStyle=aE.color;aH(aE.datapoints,aB,ae(aE.points,aE.color),0,false,aE.xaxis,aE.yaxis,aF);H.restore()}function E(aN,aM,aV,aI,aQ,aF,aD,aL,aK,aU,aR,aC){var aE,aT,aJ,aP,aG,aB,aO,aH,aS;if(aR){aH=aB=aO=true;aG=false;aE=aV;aT=aN;aP=aM+aI;aJ=aM+aQ;if(aT<aE){aS=aT;aT=aE;aE=aS;aG=true;aB=false}}else{aG=aB=aO=true;aH=false;aE=aN+aI;aT=aN+aQ;aJ=aV;aP=aM;if(aP<aJ){aS=aP;aP=aJ;aJ=aS;aH=true;aO=false}}if(aT<aL.min||aE>aL.max||aP<aK.min||aJ>aK.max){return}if(aE<aL.min){aE=aL.min;aG=false}if(aT>aL.max){aT=aL.max;aB=false}if(aJ<aK.min){aJ=aK.min;aH=false}if(aP>aK.max){aP=aK.max;aO=false}aE=aL.p2c(aE);aJ=aK.p2c(aJ);aT=aL.p2c(aT);aP=aK.p2c(aP);if(aD){aU.beginPath();aU.moveTo(aE,aJ);aU.lineTo(aE,aP);aU.lineTo(aT,aP);aU.lineTo(aT,aJ);aU.fillStyle=aD(aJ,aP);aU.fill()}if(aC>0&&(aG||aB||aO||aH)){aU.beginPath();aU.moveTo(aE,aJ+aF);if(aG){aU.lineTo(aE,aP+aF)}else{aU.moveTo(aE,aP+aF)}if(aO){aU.lineTo(aT,aP+aF)}else{aU.moveTo(aT,aP+aF)}if(aB){aU.lineTo(aT,aJ+aF)}else{aU.moveTo(aT,aJ+aF)}if(aH){aU.lineTo(aE,aJ+aF)}else{aU.moveTo(aE,aJ+aF)}aU.stroke()}}function e(aD){function aC(aJ,aI,aL,aG,aK,aN,aM){var aO=aJ.points,aF=aJ.pointsize;for(var aH=0;aH<aO.length;aH+=aF){if(aO[aH]==null){continue}E(aO[aH],aO[aH+1],aO[aH+2],aI,aL,aG,aK,aN,aM,H,aD.bars.horizontal,aD.bars.lineWidth)}}H.save();H.translate(q.left,q.top);H.lineWidth=aD.bars.lineWidth;H.strokeStyle=aD.color;var aB=aD.bars.align=="left"?0:-aD.bars.barWidth/2;var aE=aD.bars.fill?function(aF,aG){return ae(aD.bars,aD.color,aF,aG)}:null;aC(aD.datapoints,aB,aB+aD.bars.barWidth,0,aE,aD.xaxis,aD.yaxis);H.restore()}function ae(aD,aB,aC,aF){var aE=aD.fill;if(!aE){return null}if(aD.fillColor){return am(aD.fillColor,aC,aF,aB)}var aG=c.color.parse(aB);aG.a=typeof aE=="number"?aE:0.4;aG.normalize();return aG.toString()}function o(){av.find(".legend").remove();if(!O.legend.show){return}var aH=[],aF=false,aN=O.legend.labelFormatter,aM,aJ;for(var aE=0;aE<Q.length;++aE){aM=Q[aE];aJ=aM.label;if(!aJ){continue}if(aE%O.legend.noColumns==0){if(aF){aH.push("</tr>")}aH.push("<tr>");aF=true}if(aN){aJ=aN(aJ,aM)}aH.push('<td class="legendColorBox"><div style="border:1px solid '+O.legend.labelBoxBorderColor+';padding:1px"><div style="width:4px;height:0;border:5px solid '+aM.color+';overflow:hidden"></div></div></td><td class="legendLabel">'+aJ+"</td>")}if(aF){aH.push("</tr>")}if(aH.length==0){return}var aL='<table style="font-size:smaller;color:'+O.grid.color+'">'+aH.join("")+"</table>";if(O.legend.container!=null){c(O.legend.container).html(aL)}else{var aI="",aC=O.legend.position,aD=O.legend.margin;if(aD[0]==null){aD=[aD,aD]}if(aC.charAt(0)=="n"){aI+="top:"+(aD[1]+q.top)+"px;"}else{if(aC.charAt(0)=="s"){aI+="bottom:"+(aD[1]+q.bottom)+"px;"}}if(aC.charAt(1)=="e"){aI+="right:"+(aD[0]+q.right)+"px;"}else{if(aC.charAt(1)=="w"){aI+="left:"+(aD[0]+q.left)+"px;"}}var aK=c('<div class="legend">'+aL.replace('style="','style="position:absolute;'+aI+";")+"</div>").appendTo(av);if(O.legend.backgroundOpacity!=0){var aG=O.legend.backgroundColor;if(aG==null){aG=O.grid.backgroundColor;if(aG&&typeof aG=="string"){aG=c.color.parse(aG)}else{aG=c.color.extract(aK,"background-color")}aG.a=1;aG=aG.toString()}var aB=aK.children();c('<div style="position:absolute;width:'+aB.width()+"px;height:"+aB.height()+"px;"+aI+"background-color:"+aG+';"> </div>').prependTo(aK).css("opacity",O.legend.backgroundOpacity)}}}var ab=[],M=null;function K(aI,aG,aD){var aO=O.grid.mouseActiveRadius,a0=aO*aO+1,aY=null,aR=false,aW,aU;for(aW=Q.length-1;aW>=0;--aW){if(!aD(Q[aW])){continue}var aP=Q[aW],aH=aP.xaxis,aF=aP.yaxis,aV=aP.datapoints.points,aT=aP.datapoints.pointsize,aQ=aH.c2p(aI),aN=aF.c2p(aG),aC=aO/aH.scale,aB=aO/aF.scale;if(aH.options.inverseTransform){aC=Number.MAX_VALUE}if(aF.options.inverseTransform){aB=Number.MAX_VALUE}if(aP.lines.show||aP.points.show){for(aU=0;aU<aV.length;aU+=aT){var aK=aV[aU],aJ=aV[aU+1];if(aK==null){continue}if(aK-aQ>aC||aK-aQ<-aC||aJ-aN>aB||aJ-aN<-aB){continue}var aM=Math.abs(aH.p2c(aK)-aI),aL=Math.abs(aF.p2c(aJ)-aG),aS=aM*aM+aL*aL;if(aS<a0){a0=aS;aY=[aW,aU/aT]}}}if(aP.bars.show&&!aY){var aE=aP.bars.align=="left"?0:-aP.bars.barWidth/2,aX=aE+aP.bars.barWidth;for(aU=0;aU<aV.length;aU+=aT){var aK=aV[aU],aJ=aV[aU+1],aZ=aV[aU+2];if(aK==null){continue}if(Q[aW].bars.horizontal?(aQ<=Math.max(aZ,aK)&&aQ>=Math.min(aZ,aK)&&aN>=aJ+aE&&aN<=aJ+aX):(aQ>=aK+aE&&aQ<=aK+aX&&aN>=Math.min(aZ,aJ)&&aN<=Math.max(aZ,aJ))){aY=[aW,aU/aT]}}}}if(aY){aW=aY[0];aU=aY[1];aT=Q[aW].datapoints.pointsize;return{datapoint:Q[aW].datapoints.points.slice(aU*aT,(aU+1)*aT),dataIndex:aU,series:Q[aW],seriesIndex:aW}}return null}function aa(aB){if(O.grid.hoverable){u("plothover",aB,function(aC){return aC.hoverable!=false})}}function l(aB){if(O.grid.hoverable){u("plothover",aB,function(aC){return false})}}function R(aB){u("plotclick",aB,function(aC){return aC.clickable!=false})}function u(aC,aB,aD){var aE=y.offset(),aH=aB.pageX-aE.left-q.left,aF=aB.pageY-aE.top-q.top,aJ=C({left:aH,top:aF});aJ.pageX=aB.pageX;aJ.pageY=aB.pageY;var aK=K(aH,aF,aD);if(aK){aK.pageX=parseInt(aK.series.xaxis.p2c(aK.datapoint[0])+aE.left+q.left);aK.pageY=parseInt(aK.series.yaxis.p2c(aK.datapoint[1])+aE.top+q.top)}if(O.grid.autoHighlight){for(var aG=0;aG<ab.length;++aG){var aI=ab[aG];if(aI.auto==aC&&!(aK&&aI.series==aK.series&&aI.point[0]==aK.datapoint[0]&&aI.point[1]==aK.datapoint[1])){T(aI.series,aI.point)}}if(aK){x(aK.series,aK.datapoint,aC)}}av.trigger(aC,[aJ,aK])}function f(){if(!M){M=setTimeout(s,30)}}function s(){M=null;A.save();A.clearRect(0,0,G,I);A.translate(q.left,q.top);var aC,aB;for(aC=0;aC<ab.length;++aC){aB=ab[aC];if(aB.series.bars.show){v(aB.series,aB.point)}else{ay(aB.series,aB.point)}}A.restore();an(ak.drawOverlay,[A])}function x(aD,aB,aF){if(typeof aD=="number"){aD=Q[aD]}if(typeof aB=="number"){var aE=aD.datapoints.pointsize;aB=aD.datapoints.points.slice(aE*aB,aE*(aB+1))}var aC=al(aD,aB);if(aC==-1){ab.push({series:aD,point:aB,auto:aF});f()}else{if(!aF){ab[aC].auto=false}}}function T(aD,aB){if(aD==null&&aB==null){ab=[];f()}if(typeof aD=="number"){aD=Q[aD]}if(typeof aB=="number"){aB=aD.data[aB]}var aC=al(aD,aB);if(aC!=-1){ab.splice(aC,1);f()}}function al(aD,aE){for(var aB=0;aB<ab.length;++aB){var aC=ab[aB];if(aC.series==aD&&aC.point[0]==aE[0]&&aC.point[1]==aE[1]){return aB}}return -1}function ay(aE,aD){var aC=aD[0],aI=aD[1],aH=aE.xaxis,aG=aE.yaxis;if(aC<aH.min||aC>aH.max||aI<aG.min||aI>aG.max){return}var aF=aE.points.radius+aE.points.lineWidth/2;A.lineWidth=aF;A.strokeStyle=c.color.parse(aE.color).scale("a",0.5).toString();var aB=1.5*aF,aC=aH.p2c(aC),aI=aG.p2c(aI);A.beginPath();if(aE.points.symbol=="circle"){A.arc(aC,aI,aB,0,2*Math.PI,false)}else{aE.points.symbol(A,aC,aI,aB,false)}A.closePath();A.stroke()}function v(aE,aB){A.lineWidth=aE.bars.lineWidth;A.strokeStyle=c.color.parse(aE.color).scale("a",0.5).toString();var aD=c.color.parse(aE.color).scale("a",0.5).toString();var aC=aE.bars.align=="left"?0:-aE.bars.barWidth/2;E(aB[0],aB[1],aB[2]||0,aC,aC+aE.bars.barWidth,0,function(){return aD},aE.xaxis,aE.yaxis,A,aE.bars.horizontal,aE.bars.lineWidth)}function am(aJ,aB,aH,aC){if(typeof aJ=="string"){return aJ}else{var aI=H.createLinearGradient(0,aH,0,aB);for(var aE=0,aD=aJ.colors.length;aE<aD;++aE){var aF=aJ.colors[aE];if(typeof aF!="string"){var aG=c.color.parse(aC);if(aF.brightness!=null){aG=aG.scale("rgb",aF.brightness)}if(aF.opacity!=null){aG.a*=aF.opacity}aF=aG.toString()}aI.addColorStop(aE/(aD-1),aF)}return aI}}}c.plot=function(g,e,d){var f=new b(c(g),e,d,c.plot.plugins);return f};c.plot.version="0.7";c.plot.plugins=[];c.plot.formatDate=function(l,f,h){var o=function(d){d=""+d;return d.length==1?"0"+d:d};var e=[];var p=false,j=false;var n=l.getUTCHours();var k=n<12;if(h==null){h=["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"]}if(f.search(/%p|%P/)!=-1){if(n>12){n=n-12}else{if(n==0){n=12}}}for(var g=0;g<f.length;++g){var m=f.charAt(g);if(p){switch(m){case"h":m=""+n;break;case"H":m=o(n);break;case"M":m=o(l.getUTCMinutes());break;case"S":m=o(l.getUTCSeconds());break;case"d":m=""+l.getUTCDate();break;case"m":m=""+(l.getUTCMonth()+1);break;case"y":m=""+l.getUTCFullYear();break;case"b":m=""+h[l.getUTCMonth()];break;case"p":m=(k)?("am"):("pm");break;case"P":m=(k)?("AM"):("PM");break;case"0":m="";j=true;break}if(m&&j){m=o(m);j=false}e.push(m);if(!j){p=false}}else{if(m=="%"){p=true}else{e.push(m)}}}return e.join("")};function a(e,d){return d*Math.floor(e/d)}})(jQuery);(function(a){function b(k){var p={first:{x:-1,y:-1},second:{x:-1,y:-1},show:false,active:false};var m={};var r=null;function e(s){if(p.active){l(s);k.getPlaceholder().trigger("plotselecting",[g()])}}function n(s){if(s.which!=1){return}document.body.focus();if(document.onselectstart!==undefined&&m.onselectstart==null){m.onselectstart=document.onselectstart;document.onselectstart=function(){return false}}if(document.ondrag!==undefined&&m.ondrag==null){m.ondrag=document.ondrag;document.ondrag=function(){return false}}d(p.first,s);p.active=true;r=function(t){j(t)};a(document).one("mouseup",r)}function j(s){r=null;if(document.onselectstart!==undefined){document.onselectstart=m.onselectstart}if(document.ondrag!==undefined){document.ondrag=m.ondrag}p.active=false;l(s);if(f()){i()}else{k.getPlaceholder().trigger("plotunselected",[]);k.getPlaceholder().trigger("plotselecting",[null])}return false}function g(){if(!f()){return null}var u={},t=p.first,s=p.second;a.each(k.getAxes(),function(v,w){if(w.used){var y=w.c2p(t[w.direction]),x=w.c2p(s[w.direction]);u[v]={from:Math.min(y,x),to:Math.max(y,x)}}});return u}function i(){var s=g();k.getPlaceholder().trigger("plotselected",[s]);if(s.xaxis&&s.yaxis){k.getPlaceholder().trigger("selected",[{x1:s.xaxis.from,y1:s.yaxis.from,x2:s.xaxis.to,y2:s.yaxis.to}])}}function h(t,u,s){return u<t?t:(u>s?s:u)}function d(w,t){var v=k.getOptions();var u=k.getPlaceholder().offset();var s=k.getPlotOffset();w.x=h(0,t.pageX-u.left-s.left,k.width());w.y=h(0,t.pageY-u.top-s.top,k.height());if(v.selection.mode=="y"){w.x=w==p.first?0:k.width()}if(v.selection.mode=="x"){w.y=w==p.first?0:k.height()}}function l(s){if(s.pageX==null){return}d(p.second,s);if(f()){p.show=true;k.triggerRedrawOverlay()}else{q(true)}}function q(s){if(p.show){p.show=false;k.triggerRedrawOverlay();if(!s){k.getPlaceholder().trigger("plotunselected",[])}}}function c(s,w){var t,y,z,A,x=k.getAxes();for(var u in x){t=x[u];if(t.direction==w){A=w+t.n+"axis";if(!s[A]&&t.n==1){A=w+"axis"}if(s[A]){y=s[A].from;z=s[A].to;break}}}if(!s[A]){t=w=="x"?k.getXAxes()[0]:k.getYAxes()[0];y=s[w+"1"];z=s[w+"2"]}if(y!=null&&z!=null&&y>z){var v=y;y=z;z=v}return{from:y,to:z,axis:t}}function o(t,s){var v,u,w=k.getOptions();if(w.selection.mode=="y"){p.first.x=0;p.second.x=k.width()}else{u=c(t,"x");p.first.x=u.axis.p2c(u.from);p.second.x=u.axis.p2c(u.to)}if(w.selection.mode=="x"){p.first.y=0;p.second.y=k.height()}else{u=c(t,"y");p.first.y=u.axis.p2c(u.from);p.second.y=u.axis.p2c(u.to)}p.show=true;k.triggerRedrawOverlay();if(!s&&f()){i()}}function f(){var s=5;return Math.abs(p.second.x-p.first.x)>=s&&Math.abs(p.second.y-p.first.y)>=s}k.clearSelection=q;k.setSelection=o;k.getSelection=g;k.hooks.bindEvents.push(function(t,s){var u=t.getOptions();if(u.selection.mode!=null){s.mousemove(e);s.mousedown(n)}});k.hooks.drawOverlay.push(function(v,D){if(p.show&&f()){var t=v.getPlotOffset();var s=v.getOptions();D.save();D.translate(t.left,t.top);var z=a.color.parse(s.selection.color);D.strokeStyle=z.scale("a",0.8).toString();D.lineWidth=1;D.lineJoin="round";D.fillStyle=z.scale("a",0.4).toString();var B=Math.min(p.first.x,p.second.x),A=Math.min(p.first.y,p.second.y),C=Math.abs(p.second.x-p.first.x),u=Math.abs(p.second.y-p.first.y);D.fillRect(B,A,C,u);D.strokeRect(B,A,C,u);D.restore()}});k.hooks.shutdown.push(function(t,s){s.unbind("mousemove",e);s.unbind("mousedown",n);if(r){a(document).unbind("mouseup",r)}})}a.plot.plugins.push({init:b,options:{selection:{mode:null,color:"#e8cfac"}},name:"selection",version:"1.1"})})(jQuery);/*
 * jquery.flot.tooltip
 *
 * desc:	create tooltip with values of hovered point on the graph, 
					support many series, time mode, stacking and pie charts
					you can set custom tip content (also with use of HTML tags) and precision of values
 * version:	0.4.4
 * author: 	Krzysztof Urbas @krzysu [myviews.pl] with help of @ismyrnow
 * website:	https://github.com/krzysu/flot.tooltip
 * 
 * released under MIT License, 2012
*/

(function ($) {
	var options = {
		tooltip: false, //boolean
		tooltipOpts: {
			content: "%s | X: %x | Y: %y.2", //%s -> series label, %x -> X value, %y -> Y value, %x.2 -> precision of X value, %p -> percent
			dateFormat: "%y-%0m-%0d",
			shifts: {
				x: 10,
				y: 20
			},
			defaultTheme: true
		}
	};

	var init = function(plot) {

		var tipPosition = {x: 0, y: 0};
		var opts = plot.getOptions();
		
		var updateTooltipPosition = function(pos) {
			tipPosition.x = pos.x;
			tipPosition.y = pos.y;
		};
		
		var onMouseMove = function(e) {
						
			var pos = {x: 0, y: 0};

			pos.x = e.pageX;
			pos.y = e.pageY;

			updateTooltipPosition(pos);
		};
		
		var timestampToDate = function(tmst) {

			var theDate = new Date(tmst);
			
			return $.plot.formatDate(theDate, opts.tooltipOpts.dateFormat);
		};
		
		plot.hooks.bindEvents.push(function (plot, eventHolder) {
            
			var to = opts.tooltipOpts;
			var placeholder = plot.getPlaceholder();
			var $tip;
			
			if (opts.tooltip === false) return;

			if( $('#flotTip').length > 0 ){
				$tip = $('#flotTip');
			}
			else {
				$tip = $('<div />').attr('id', 'flotTip');
				$tip.appendTo('body').hide().css({position: 'absolute'});
			
				if(to.defaultTheme) {
					$tip.css({
						'background': '#fff',
						'z-index': '100',
						'padding': '0.4em 0.6em',
						'border-radius': '0.5em',
						'font-size': '0.8em',
						'border': '1px solid #111'
					});
				}
			}
			
			$(placeholder).bind("plothover", function (event, pos, item) {
				if (item) {					
					var tipText;

					if(opts.xaxis.mode === "time" || opts.xaxes[0].mode === "time") {
						tipText = stringFormat(to.content, item, timestampToDate);
					}
					else {
						tipText = stringFormat(to.content, item);						
					}
					
					$tip.html( tipText ).css({left: tipPosition.x + to.shifts.x, top: tipPosition.y + to.shifts.y}).show();
				}
				else {
					$tip.hide().html('');
				}
			});
			
			eventHolder.mousemove(onMouseMove);
		});
		
		var stringFormat = function(content, item, fnct) {
		
			var percentPattern = /%p\.{0,1}(\d{0,})/;
			var seriesPattern = /%s/;
			var xPattern = /%x\.{0,1}(\d{0,})/;
			var yPattern = /%y\.{0,1}(\d{0,})/;
			
			//percent match
			if( typeof (item.series.percent) !== 'undefined' ) {
				content = adjustValPrecision(percentPattern, content, item.series.percent);
			}
			//series match
			if( typeof(item.series.label) !== 'undefined' ) {
				content = content.replace(seriesPattern, item.series.label);
			}
			// xVal match
			if( typeof(fnct) === 'function' ) {
				content = content.replace(xPattern, fnct(item.series.data[item.dataIndex][0]) );
			}
			else if( typeof item.series.data[item.dataIndex][0] === 'number' ) {
				content = adjustValPrecision(xPattern, content, item.series.data[item.dataIndex][0]);
			}
			// yVal match
			if( typeof item.series.data[item.dataIndex][1] === 'number' ) {
				content = adjustValPrecision(yPattern, content, item.series.data[item.dataIndex][1]);
			}

			return content;
		};
		
		var adjustValPrecision = function(pattern, content, value) {
		
			var precision;
			if( content.match(pattern) !== 'null' ) {
				if(RegExp.$1 !== '') {
					precision = RegExp.$1;
					value = value.toFixed(precision)
				}
				content = content.replace(pattern, value);
			}
		
			return content;
		};
	}
    
	$.plot.plugins.push({
		init: init,
		options: options,
		name: 'tooltip',
		version: '0.4.4'
	});
})(jQuery);
