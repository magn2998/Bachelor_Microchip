const CMD_SOF = '>';
const CMD_DELIM_HEX = '#';
const CMD_DELIM_BIN = '%';
const CMD_VERS = 'V';
const CMD_SEND = 'S';
const CMD_UNZIP = 'Z';
const CMD_DATA = 'D';
const CMD_AUTH = 'U';
const CMD_STRAP = 'O';
const CMD_OTPD = 'P';
const CMD_OTPR = 'R';
const CMD_OTPC = 'M';
const CMD_OTP_REGIONS = 'G';
const CMD_CONT = 'C';
const CMD_SJTAG_RD = 'Q';
const CMD_SJTAG_WR = 'A';
const CMD_ACK = 'a';
const CMD_NACK = 'n';
const CMD_TRACE = 'T';
const CMD_BL2U_WRITE = 'W';
const CMD_BL2U_IMAGE = 'I';
const CMD_BL2U_BIND ='B';
const CMD_BL2U_OTP_READ_RAW ='l';
const CMD_BL2U_OTP_READ_EMU ='L';
const CMD_BL2U_RESET ='e';

const CMD_ENABLE_CACHE = 'j';
const CMD_DISABLE_CACHE = 'J';

const CMD_MEMORYCONFIG_READ = 'h';
const CMD_MEMORYCONFIG_INIT_CUSTOM = 'f';

const CMD_MEMORYTEST_BURSTWRITE = 'H';

const CMD_MEMORYTEST_RND = 'x';
const CMD_MEMORYTEST_RND_REV = 'X';
const CMD_MEMORYTEST_ONES = 'y';
const CMD_MEMORYTEST_ONES_REV = 'Y';
const CMD_MEMORYTEST_ADDRESS = 'p';
const CMD_MEMORYTEST_ADDRESS_REV = 'q';
const CMD_MEMORYTEST_DATABUS = 'k';
const CMD_MEMORYTEST_ADDRBUS = 'K';
const CMD_MEMORYTEST_HAMMER = 'w';
const CMD_MEMORYTEST_BITFADE = 'F';
const CMD_MEMORYTEST_BITFADE_ALLZEROS = 'E';

const CMD_MEMORYTEST_CUSTOM = 'g';

var globalVar;
var timeObject = {};

// Interface Object - contains all keys neccessary for the ddr config
let ddr_config_interface ={
	info: {
		version: '',
		speed: '', 
		size: '', 
		bus_width: '',  
	},
	main: {
		dfimisc: '',
		dfitmg0: '',
		dfitmg1: '',
		dfiupd0: '',
		dfiupd1: '',
		ecccfg0: '',
		init0: '',
		init1: '',
		init3: '',
		init4: '',
		init5: '',
		mstr: '',
		pccfg: '',
		pwrctl: '',
		rfshctl0: '',
		rfshctl3: '',
	},

	timing: {
		dramtmg0: '',
		dramtmg1: '',
		dramtmg2: '',
		dramtmg3: '',
		dramtmg4: '',
		dramtmg5: '',
		dramtmg8: '',
		odtcfg: '',
		rfshtmg: '',
	},

	mapping: {
		addrmap0: '',
		addrmap1: '',
		addrmap2: '',
		addrmap3: '',
		addrmap4: '',
		addrmap5: '',
		addrmap6: '',
	},

	phy: {
		dcr: '',
		dsgcr: '',
		dtcr: '',
		dxccr: '',
		pgcr2: '',
		zq0cr0: '',
		zq0cr1: '',
		zq1cr0: '',
		zq1cr1: '',
	},

	phy_timing: {
		dtpr0: '',
		dtpr1: '',
		dtpr2: '',
		mr0: '',
		mr1: '',
		mr2: '',
		mr3: '',
		ptr0: '',
		ptr1: '',
		ptr2: '',
		ptr3: '',
		ptr4: '',
	}
}

let countdown_times = {
	wo_cache: {
		[CMD_MEMORYTEST_DATABUS]:     0.011375,
		[CMD_MEMORYTEST_ADDRBUS]:     0.01387,
		[CMD_MEMORYTEST_RND]:         30.5146,
		[CMD_MEMORYTEST_RND_REV]:     68.6747,
		[CMD_MEMORYTEST_ONES]:        27.6234,
		[CMD_MEMORYTEST_ONES_REV]:    65.8197,
		[CMD_MEMORYTEST_ADDRESS]:     27.5571,
		[CMD_MEMORYTEST_ADDRESS_REV]: 64.6093,
		[CMD_MEMORYTEST_BITFADE]:           26.1613,
		[CMD_MEMORYTEST_BITFADE_ALLZEROS]:  26.1646,
		[CMD_MEMORYTEST_HAMMER]:      147.3978
	},
	w_cache: {
		[CMD_MEMORYTEST_DATABUS]:     0.016384615,
		[CMD_MEMORYTEST_ADDRBUS]:     0.01885,
		[CMD_MEMORYTEST_RND]:         5.9182,
		[CMD_MEMORYTEST_RND_REV]:     9.0488,
		[CMD_MEMORYTEST_ONES]:        4.3297,
		[CMD_MEMORYTEST_ONES_REV]:    6.6339,
		[CMD_MEMORYTEST_ADDRESS]:     3.8897,
		[CMD_MEMORYTEST_ADDRESS_REV]: 5.7575,
		[CMD_MEMORYTEST_BITFADE]:          2.312,
		[CMD_MEMORYTEST_BITFADE_ALLZEROS]: 2.310,
		[CMD_MEMORYTEST_BURSTWRITE]:  0.0402
	}	
};

let cur_stage = "connect";	// Initial "tab"
let tracing = false;
let port_reader;

const otp_max_offset = 8192;
const otp_max_read = 256;

const otp_flag_rnd = 0x01;
const otp_flag_set = 0x02;

const otp_fields = [
    //{"name": 'OTP_PRG',			"offset": 0, "size": 4, },
    {"name": 'FEAT_DIS',		"offset": 4, "size": 1, },
    {"name": 'PARTID',			"offset": 5, "size": 2, },
    {"name": 'TST_TRK',			"offset": 7, "size": 1, },
    {"name": 'SERIAL_NUMBER',		"offset": 8, "size": 8, },
    {"name": 'SECURE_JTAG',		"offset": 16, "size": 4, },
    {"name": 'WAFER_TRK',		"offset": 20, "size": 7, },
    {"name": 'JTAG_UUID',		"offset": 32, "size": 10, },
    {"name": 'TRIM',			"offset": 48, "size": 8, },
    {"name": 'PROTECT_OTP_WRITE',	"offset": 64, "size": 4, },
    {"name": 'PROTECT_REGION_ADDR',	"offset": 68, "size": 32, },
    {"name": 'PCIE_FLAGS',		"offset": 100, "size": 4, },
    {"name": 'PCIE_DEV',		"offset": 104, "size": 4, },
    {"name": 'PCIE_ID',			"offset": 108, "size": 8, },
    {"name": 'PCIE_BARS',		"offset": 116, "size": 40, },
    {"name": 'Root of Trust (ROTPK)',	"offset": 256, "size": 32,	"use": otp_flag_set, },
    {"name": 'Private key (HUK)',	"offset": 288, "size": 32,	"use": otp_flag_rnd, },
    {"name": 'Endorsement key (EK)',	"offset": 320, "size": 32,	"use": otp_flag_set, },
    {"name": 'Shared key (SSK)',	"offset": 352, "size": 32,	"use": otp_flag_set, },
    {"name": 'SJTAG Key',		"offset": 384, "size": 32,	"use": otp_flag_set, },
    {"name": 'STRAP disable mask',	"offset": 420, "size": 2, 	"use": otp_flag_set, },
    {"name": 'TBBR_NTNVCT',		"offset": 512, "size": 32, },
    {"name": 'TBBR_TNVCT',		"offset": 544, "size": 32, },
]

const platforms = {
    "LAN966X B0": "v2.4(release):laguna-transplant-base-v0-23-g225acd2",
    "LAN969X SR": "v2.6(debug):v2.6-578-gb8ba4e297b19-dirty",
};

function validResponse(r)
{
    var m = r.match(/>(\w),([0-9a-f]{8}),([0-9a-f]{8})(#|%)(.+)/i);
    //console.log(m);
    return m;
}

// Generates a request to send over the DDR configuration as a HEX string
function format_ddr_config_to_data_request(ddrConfig) {
	let ddrConfig_array = DDRconfigToArray(ddrConfig);

	let req = CMD_DATA + ',' + fmtHex(1);
	req += ',' + fmtHex(128+ddrConfig_array.data.length*4); // Each field is 4 bytes long and the title is 128
	req += CMD_DELIM_HEX;
	// First add the string to the request
	for(let i = 0; i < ddrConfig_array.name.length; i++) {
		req += ddrConfig_array.name.charCodeAt(i).toString(16).padStart(2, "0");
	}
	// Add zeros for the remaning bytes not used in the name string
	for(let i = 0; i < 128 - ddrConfig_array.name.length; i++) { 
		req += "00";
	}
	// Adds the remaining 224 bytes aka. 56 4 byte integers
	for(let i = 0; i < ddrConfig_array.data.length; i++) { 
		req += BigEndianToSmallEndianHexString(ddrConfig_array.data[i].toString(16).padStart(8,"0"));
	}

    return req;
}

function format_customPattern_to_hexString(instructionArray) {
	let req = CMD_DATA + ',' + fmtHex(1);
	req += ',' + fmtHex(256); // The custom pattern program is max 256 bytes long (64 instructions)
	req += CMD_DELIM_HEX;
	// Add the instructions formatted correctly
	for(let i = 0; i < instructionArray.length; i++) { 
		req += BigEndianToSmallEndianHexString(instructionArray[i].toString(16).padStart(8,"0"));
	}

    return req;
}

function format_uint32_to_reqString(val) {
	let req = CMD_DATA + ',' + fmtHex(1);
	req += ',' + fmtHex(4); // The value is a single 32 bit (4 byte) value
	req += CMD_DELIM_HEX;
	req += BigEndianToSmallEndianHexString(val.toString(16).padStart(8,"0"));
    return req;
}


function DDRconfigToArray(ddrConfig) {
	let ddrArray = {
		name: ddrConfig.info.version,
		data: [
			parseInt(ddrConfig.info.speed),
			parseInt((ddrConfig.info.size === undefined ? ddrConfig.info.mem_size_mb : ddrConfig.info.size)), // Size can either be given as MB or simply bytes.. this covers both cases
			parseInt(ddrConfig.info.bus_width)
		] 
	}

	// Push every data field in the correct order
	PushKeysInOrderToArray(ddrConfig.main);
	PushKeysInOrderToArray(ddrConfig.timing);
	PushKeysInOrderToArray(ddrConfig.mapping);
	PushKeysInOrderToArray(ddrConfig.phy);
	PushKeysInOrderToArray(ddrConfig.phy_timing);
	
	return ddrArray;
	function PushKeysInOrderToArray(obj) {
		let keys = Object.keys(obj).sort(); // Get keys and sort them
		for(var key of keys) {
			ddrArray.data.push(parseInt(obj[key]));
		}
	}
}

// Does the inverse of function DDRconfigToArray
function DDRarrayToConfig(dataStr) {
	// Begin by reading the version / name
	ddr_config_interface.info.version = '';
	for(let i = 0; i < 128; i++) { // Name is max 128 characters long
		if(dataStr[i] === '\0') { // If it is null character => end of string
			break;
		}
		ddr_config_interface.info.version += dataStr[i];
	}
	ddr_config_interface.info.speed     = ReadFieldFromDataArray(dataStr, 128);
	ddr_config_interface.info.size      = ReadFieldFromDataArray(dataStr, 132);
	ddr_config_interface.info.bus_width = ReadFieldFromDataArray(dataStr, 136);

	let newIdx = LoadValueIntoObject(ddr_config_interface.main, dataStr, 140);
		newIdx = LoadValueIntoObject(ddr_config_interface.timing, dataStr, newIdx);
		newIdx = LoadValueIntoObject(ddr_config_interface.mapping, dataStr, newIdx);
		newIdx = LoadValueIntoObject(ddr_config_interface.phy, dataStr, newIdx);
		newIdx = LoadValueIntoObject(ddr_config_interface.phy_timing, dataStr, newIdx);

	return ddr_config_interface;
	function LoadValueIntoObject(obj, dataStr, idx) {
		let keys = Object.keys(obj).sort();
		for(let i = 0; i < keys.length; i++) {
			obj[keys[i]] = ReadFieldFromDataArray(dataStr, idx+i*4);
		}
		return idx+keys.length*4;
	}	
}

// Reads a 4 byte data field from a given indes of a data array
function ReadFieldFromDataArray(dataStr, idx) {
	let char1 = dataStr.charCodeAt(idx); // Read first character
	let char2 = dataStr.charCodeAt(idx+1); // Read second character
	let char3 = dataStr.charCodeAt(idx+2); // Read third character
	let char4 = dataStr.charCodeAt(idx+3); // Read fourth character
	let val = 0 | char4;
	val = (val << 0x8) | char3;
	val = (val << 0x8) | char2;
	return ((val << 0x8) | char1) >>> 0; // Unsigned right shift - converts signed Javascript integer into unsigned
}

function BigEndianToSmallEndianHexString(hexString) { // E.g. 0x12345bcf => 0xcf5b3412
	return hexString[6] + hexString[7] + hexString[4] + hexString[5] + hexString[2] + hexString[3] + hexString[0] + hexString[1]; 
}

class BootstrapRequestTransformer {
    constructor() {
	// A container for holding stream data until a new line.
	this.chunks = "";
	this.sync = false;	// Received '>' already
	// Length of request "fixed" parts
	this.fixedlen = ">X,00000000,00000000#00000000".length;
    }

    transform(chunk, controller) {
	// Append new chunks to existing chunks.
	this.chunks += chunk;

	if (!this.sync) {
	    var sync_ix = chunk.indexOf('>');
	    var skipped = "";
	    if (sync_ix != -1) {
		skipped = chunk.substr(0, sync_ix - 1);
		this.chunks = chunk.substr(sync_ix);
		this.sync = true;
		if (tracing)
		    console.log("Sync: %s", this.chunks);
	    } else {
		skipped = chunk;
	    }
	    if (skipped.length)
		console.log("Skipped: %s", skipped);
	}

	if (this.sync) {
	    if (tracing)
		console.log("Xform: %s -> %s", chunk, this.chunks);
	    // Enough data to have a real request?
	    if (this.chunks.length >= this.fixedlen) {
		var rmatch = validResponse(this.chunks);
		if (rmatch) {
		    var datalen = parseInt(rmatch[3], 16);
		    var reqlen = (datalen * 2) + this.fixedlen;
		    if (tracing)
			console.log("valid: datalen %d, reqlen %d", datalen, reqlen);
		    if (this.chunks.length >= reqlen) {
			controller.enqueue(this.chunks.substr(0, reqlen));
			this.sync = false;
			this.chunks = this.chunks.substr(reqlen);
		    } else {
			if (tracing)
			    console.log("short: datalen %d, reqlen %d, has %d", datalen,
					reqlen, this.chunks.length);
		    }
		}
	    }
	}
    }

    flush(controller) {
	// When the stream is closed, flush any remaining chunks out.
	this.chunks = "";
	this.sync = false;
    }
}

function getKeyByValue(object, value)
{
    return Object.keys(object).find(key => object[key] === value);
}

function fmtHex(arg)
{
    var h = (arg >>> 0).toString(16).padStart(8, "0");
    return h;
}

function ntohl(arg)
{
    var ret = "";
    for (var i = 24; i >= 0; i -= 8)
	ret += String.fromCharCode(arg >> i);
    return ret;
}

function hexString2Bin(s)
{
    var bytes = s.split(":");
    var str = "";
    for (var i = 0; i < bytes.length; i++) {
	var b = bytes[i];
	if (b.match(/^[0-9a-f]{2}$/i)) {
	    str += String.fromCharCode(parseInt(b, 16));
	} else {
	    throw "Illegal data: " + b;
	}
    }
    return str;
}

function encodeString2Array(buf)
{
    var bArr = new Uint8Array(buf.length);
    for (var i = 0; i < buf.length; i++) {
	bArr[i] = buf.charCodeAt(i);
    }
    return bArr;
}

function fmtReq(cmdName, arg, data, binary)
{
    var buf = cmdName + ',' + fmtHex(arg);
    if (data) {
	let encodedData = "";
	buf += ',' + fmtHex(data.length);
	if (binary) {
	    buf += CMD_DELIM_BIN;
	    for (var i = 0; i < data.length; i++) {
		encodedData += String.fromCharCode(data.charCodeAt(i));
	    }
	} else {
	    buf += CMD_DELIM_HEX;
	    for (var i = 0; i < data.length; i++) {
		encodedData += data.charCodeAt(i).toString(16).padStart(2, "0");
	    }
	}
	buf += encodedData;
    } else {
	buf += ',' + fmtHex(0) + CMD_DELIM_HEX;
    }
    return buf;
}

function decodeData(type, data)
{
    if (type == '#') {		// hex encoded
	var result = "";
	while (data.length >= 2) {
            result += String.fromCharCode(parseInt(data.substring(0, 2), 16));
            data = data.substring(2, data.length);
	}
	return result;
    } else if (type == '%') {	// Raw binary
	return data;
    } else {
	throw "Invalid data type" + type;
    }
}

function parseResponse(input)
{
    var m = input.match(/>(\w),([0-9a-f]{8}),([0-9a-f]{8})(#|%)(.+)$/i);
    //console.log(m);
    if (m) {
	var r = m[0];
	var tail = m[5];
	var tailcrc = tail.substr(-8).toLowerCase();
	var resp = {
	    'command': m[1],
	    'arg':     parseInt(m[2], 16),
	    'length':  parseInt(m[3], 16),
	};
	// Zoom into CRC32 data
	var pdulen = 1 + 1 + 2 * (8 + 1) + 8 + (resp["length"] * 2);
	var data = r.substr(1, pdulen - 8); // Pdu ex crc
	crc32 = fmtHex(CRC32C.str(data)).toLowerCase();
	//console.log("Calc: " + crc32 + " - Rcv: " + tailcrc);
	if (crc32 == tailcrc) {
	    resp["crc"] = crc32;
	    resp["data"] = decodeData(m[4], tail.substr(0, tail.length - 8));
	} else {
	    console.log("CRC error: %s", r);
	    console.log("CRC error: %s", data);
	    console.log("CRC error: Have %s want %s", tailcrc, crc32);
	    throw "CRC error";
	}
	if (resp["length"] != resp["data"].length) {
	    throw "Length sanity error";
	}
	return resp;
    }
    return null;
}

async function portOpen(port)
{
    await port.open({ baudRate: 115200});
    const textDecoder = new TextDecoderStream();
    const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
    const reader = textDecoder.readable
	  .pipeThrough(new TransformStream(new BootstrapRequestTransformer()))
	  .getReader();

    return reader;
}

async function sendRequest(port, buf)
{
    var outArray = encodeString2Array(buf);
    var crc32 = fmtHex(CRC32C.buf(outArray));
    if (tracing)
	console.log("Req: " + buf + crc32);
    const encoder = new TextEncoder();
    const writer = port.writable.getWriter();
    await writer.write(encoder.encode(CMD_SOF));
    await writer.write(outArray);
    await writer.write(encoder.encode(crc32));
    writer.releaseLock();
}

async function completeRequest(port, req_str)
{
    // Signal busy
    document.getElementById("active").style.display = 'inline';
    // Send request
    await sendRequest(port, req_str);
    // Get response from port_reader stream
    let res = await readRequest();
    // Signal un-busy
    document.getElementById("active").style.display = 'none';
    // Return result
    return res;
}

async function readRequest() {
	var response = await port_reader.read();
    //console.log("Response: %o", response);
    // Signal received req
    
    try {
	var rspStruct = parseResponse(response.value);
	//console.log("Response: %o", rspStruct);
	if (rspStruct["command"] == CMD_ACK) {
	    //console.log("Command acked: %d, crc = %s", rspStruct["arg"], rspStruct["crc"]);
	    return rspStruct;
	} else if (rspStruct["command"] == CMD_NACK) {
	    console.log("NACK: %o", rspStruct);
	    throw rspStruct["data"] ? rspStruct["data"] : "Unspecific NACK";
	} else {
	    console.log("Request failed to ack: %o", rspStruct);
	    throw "Request failed to ack";
	}
    } catch(e) {
		console.log("completeRequest: " + e);
		throw e;
    }
    return null;
}

async function downloadApp(port, appdata, binary)
{
    var completed = true;
    setStatus("Downloading " + appdata.length + " bytes " + (binary ? "binary" : "hex encoded") );
    try {
	const chunkSize = 256;
	let bytesSent = 0;

	await completeRequest(port, fmtReq(CMD_SEND, appdata.length));

	// Send data chunks
	while (bytesSent < appdata.length) {
	    let chunk = appdata.substr(bytesSent, chunkSize);
	    //console.log("Sending at offset: %d", bytesSent);
		let req = fmtReq(CMD_DATA, bytesSent, chunk, binary);
	    await completeRequest(port, req);
	    bytesSent += chunk.length;
	    if (bytesSent % (5 * 1024))
		setStatus("Sent " + bytesSent + " bytes (" + (bytesSent * 100 / appdata.length).toFixed().toString(10) + "%)", true);
	    //console.log("Sent bytes: %d", chunk.length);
	}
    } catch (e) {
	addTrace("Download failed: " + e);
	completed = false;
    }
    setStatus("Download was " + (completed ? "completed" : "aborted"));
    return completed;
}

async function delayWait(timeout)
{
    return new Promise(resolve => setTimeout(resolve, timeout));
}

async function delaySkipInput(port, timeout)
{
    await delayWait(timeout);
}

function addTrace(s)
{
    var trc = document.getElementById('log');
    console.log(s);
    trc.textContent += s + '\n'
    trc.scrollTop = trc.scrollHeight; // Scroll down
}

function setStatus(s, notrace)
{
    if (!notrace)
	addTrace(s);
    document.getElementById('status').innerHTML = s;
}

function setOpStatus(id, s)
{
    document.getElementById(id).innerHTML = s;
}

function setFeedbackStatus(id, s)
{
    console.log(s);
    setStatus(s);
    setOpStatus(id, s);
}

async function doWrite(port, operation, cmd, dev, status_id)
{
    let s = disableButtons("bl2u", true);
    try {
	setFeedbackStatus(status_id, "Starting " + operation);
	addTrace("This may take up to 5 minutes or even longer depending on data size and media.");
	let write = await completeRequest(port, fmtReq(cmd, dev));
	setFeedbackStatus(status_id, "Completed " + operation);
    } catch(e) {
	setFeedbackStatus(status_id, "Failed " + operation + ": " + e);
    } finally {
	restoreButtons(s);
    }
}

function disableButtons(stage, disable)
{
    let div = document.getElementById(stage);
    if (div) {
	let aBtn = div.querySelectorAll("button, input, select");
	let state = new Map();
	for (var btn of aBtn) {
	    state.set(btn.id, btn.disabled);
	    btn.disabled = disable;
	}
	return state;
    }
    return null;
}

function restoreButtons(state)
{
    for (let [key, value] of state) {
	let btn = document.getElementById(key);
	if (btn)
	    btn.disabled = value;
    }
}

function setStage(new_stage)
{
    var reset_state = (cur_stage != "adv_settings");
    var stages = ["connect", "bl1", "bl2u", "adv_settings", "booting"];
    for (const stage of stages) {
	var display = (new_stage == stage ? "block" : "none");
	document.getElementById(stage).style.display = display;
    };
    if (reset_state) {
	if (new_stage == "bl1") {
	    document.getElementById('bl1_sjtag_unlock').disabled = true;
	} else {
	    disableButtons(new_stage, false);
	}
    }
    cur_stage = new_stage;
}

async function doOtpRandom(port, stage, fb_id, sel_id)
{
	let s = disableButtons(stage, true);
	try {
	    setOpStatus(fb_id, "");
	    var fld = otp_fields[document.getElementById(sel_id).value];
	    var off = fld["offset"];
	    var len = fld["size"];
	    if (!confirm("Programming the '" + fld["name"] + "' is an irreversible action, " +
			 "which will affect the operation of your device!\n\n" +
                         "Can you confirm this?"))
		throw "OTP write aborted by user cancel";
	    var rspStruct = await completeRequest(port, fmtReq(CMD_OTPR, off, ntohl(len), false));
	    setStatus("OTP set random data completed");
	    setOpStatus(fb_id, "Wrote random data to the " + fld["name"]);
	} catch(e) {
	    setStatus(e);
	    setOpStatus(fb_id, e);
	} finally {
	    restoreButtons(s);
	}
}

async function doOtpSetData(port, stage, fb_id, sel_id, data_id)
{
	let s = disableButtons(stage, true);
	try {
	    setOpStatus(fb_id, "");
	    var fld = otp_fields[document.getElementById(sel_id).value];
	    var off = fld["offset"];
	    var len = fld["size"];
	    var buf = document.getElementById(data_id).value;
	    var data = hexString2Bin(buf);
	    if (data.length != len) {
		document.getElementById(data_id).focus();
		setOpStatus(fb_id, "Data should have " + len + " data bytes.");
	    } else {
		if (!confirm("Programming the '" + fld["name"] + "' is an irreversible action, " +
			     "which will affect the operation of your device!\n\n" +
                             "Can you confirm this?"))
		    throw "OTP write aborted by user cancel";
		var rspStruct = await completeRequest(port, fmtReq(CMD_OTPD, off, data, false));
		setStatus("OTP write completed");
		setOpStatus(fb_id, "Wrote data to the " + fld["name"]);
		addTrace("OTP set '" + fld["name"] + "' = '" + buf + "'");
	    }
	} catch(e) {
	    setStatus("OTP Set: " + e);
	    setOpStatus(fb_id, e);
	} finally {
	    restoreButtons(s);
	}
}

function otpSelPopulate(name, usemask)
{
    const sel = document.getElementById(name);
    if (sel) {
	for (var i = 0; i < otp_fields.length; i++) {
	    f = otp_fields[i];
	    use = f["use"] || 0;
	    if (usemask == undefined || (use & usemask))
		sel.options[sel.options.length] = new Option(f["name"], i);
	}
    }
}

window.addEventListener("load", (event) => {
    otpSelPopulate("bl1_otp_set_rnd_fld", otp_flag_rnd);
    otpSelPopulate("bl1_otp_set_data_fld", otp_flag_set);
    otpSelPopulate("bl2u_otp_set_rnd_fld", otp_flag_rnd);
    otpSelPopulate("bl2u_otp_set_data_fld", otp_flag_set);
    otpSelPopulate("bl2u_otp_read_fld");
});

function browserCheck()
{
    if (!navigator.userAgentData)
	return false;

    for (var b of navigator.userAgentData.brands) {
	console.log(b);
	if (b.brand.match(/chrome|chromium|crios/i)){
            return true;
	} else if (b.brand.match(/edg/i)){
            return true;
	}
    }

    return false;
}

function startSerial()
{
    const app = atob(lan966x_b0_app.join(""));
    var port;
    var image;
    var sjtag_challenge;
    var settings_prev_stage;

    if (!browserCheck()) {
	document.getElementById('browser_check').style.display = 'block';
	document.getElementById('connect').style.display = 'none';
	console.log("Browser check failed, bailing out. Use Chrome or Edge.");
	return;
    }

    document.getElementById("file_select").addEventListener("change", function () {
	if (this.files && this.files[0]) {
	    var myFile = this.files[0];
	    var reader = new FileReader();

	    reader.addEventListener('load', function (e) {
		console.log("Read Image: %d bytes", e.total);
		image = e.target.result;
	    });

	    reader.readAsBinaryString(myFile);
	}
    });

    document.getElementById('bl2u_upload').addEventListener('click', async () => {
	if (image.length) {
	    let s = disableButtons("bl2u", true);
	    try {
			await downloadApp(port, image, document.getElementById("binary").checked);
			// Do explicit uncompress
			setStatus("Decompressing");
			var rspStruct = await completeRequest(port, fmtReq(CMD_UNZIP, 0));
			var datalen = rspStruct["arg"].toString(16).padStart(8, "0");
			var status = rspStruct["data"];
			setStatus("Data received: " + status + ", length " + rspStruct["arg"].toString(10));
	    } catch(e) {
			setStatus("Failed upload: " + e);
	    } finally {
			restoreButtons(s);
	    }
	}
    });

    document.getElementById('bl2u_bind').addEventListener('click', async () => {
	let s = disableButtons("bl2u", true);
	try {
	    if (!confirm("In order to perform 'Bind', the FIP image\n" +
			 "used must have been encrypted using the 'SSK' key.\n" +
			 "Can you confirm this?"))
		throw "Bind aborted by user cancel";
	    let bind = await completeRequest(port, fmtReq(CMD_BL2U_BIND, 0));
	    s.set('bl2u_bind', true); // Bind is a 'once' operation
	    setStatus("Bind completed sucessfully");
	} catch(e) {
	    setFeedbackStatus('bl2u_bind_fip_feedback', e);
	} finally {
	    restoreButtons(s);
	}
    });

    document.getElementById('bl2u_write_fip').addEventListener('click', async () => {
	let dev = document.getElementById('bl2u_write_fip_device');
	let op = "Write FIP to " + dev.selectedOptions[0].text;
	doWrite(port, op, CMD_BL2U_WRITE, dev.value, 'bl2u_write_fip_feedback');
    });

    document.getElementById('bl2u_write_image').addEventListener('click', async () => {
	let dev = document.getElementById('bl2u_write_image_device');
	let op = "Write Image to " + dev.selectedOptions[0].text;
	if (confirm("Programming flash image will not check whether " +
		    "the image contain valid data for the chosen device type.\n\n" +
                    "Can you confirm you want to perform this operation?"))
	    doWrite(port, op, CMD_BL2U_IMAGE, dev.value, 'bl2u_write_image_feedback');
    });

    document.getElementById('bl2u_otp_read').addEventListener('click', async () => {
	let s = disableButtons("bl2u", true);
	try {
	    setOpStatus("bl2u_otp_read_feedback", "");
	    var fld = otp_fields[document.getElementById('bl2u_otp_read_fld').value];
	    var off = fld["offset"];
	    var len = fld["size"];
	    var cmd = document.getElementById("bl2u_otp_read_type").selectedIndex == 0 ?
		CMD_BL2U_OTP_READ_RAW : CMD_BL2U_OTP_READ_EMU;
	    var type = (cmd == CMD_BL2U_OTP_READ_RAW ? "Raw" : "Emulated");
	    var rspStruct = await completeRequest(port, fmtReq(cmd, off, ntohl(len), false));
	    setStatus(type + " OTP read completed", true);
	    // Convert data to hex string for display
	    var data = rspStruct["data"].split('').map(function (ch) {
		return ch.charCodeAt(0).toString(16).padStart(2, "0");
	    }).join(":");
	    if (len < 32) {
		setOpStatus("bl2u_otp_read_feedback", type + ": " + data);
	    } else {
		setOpStatus("bl2u_otp_read_feedback", fld["name"] + " data is in trace");
	    }
	    addTrace(type + " OTP read " + fld["name"] + " = " + data);
	} catch(e) {
	    setStatus("OTP Read: " + e);
	    setOpStatus("bl2u_otp_read_feedback", e);
	} finally {
	    restoreButtons(s);
	}
    });

    document.getElementById('bl2u_otp_set_rnd').addEventListener('click', async () => {
	await doOtpRandom(port, 'bl2u', 'bl2u_otp_set_rnd_feedback', 'bl2u_otp_set_rnd_fld');
    });

    document.getElementById('bl2u_otp_set_data').addEventListener('click', async () => {
	await doOtpSetData(port, 'bl2u', 'bl2u_otp_set_data_feedback', 'bl2u_otp_set_data_fld', 'bl2u_otp_set_data_buf');
    });

    document.getElementById('bl2u_reset').addEventListener('click', async () => {
		let s = disableButtons("bl2u", true);
		try {
		    setStatus("Rebooting from BL2U back to BL1");
		    let cont = await completeRequest(port, fmtReq(CMD_BL2U_RESET, 0));
		    setStatus("Booting into BL1");
		    await delaySkipInput(port, 2000);
		    setStage("bl1");
		} finally {
		    restoreButtons(s);
		}
    });

    

	


	const enableMemoryTestSection = ()=>{
		let inputIds = ["memorytest_dataBus", "memorytest_dataBus_reps", "memorytest_addressBus", "memorytest_chip_hammer", "memorytest_hammer_reps", "memorytest_addrBus_reps", "memorytest_chip_rnd", "memorytest_chip_address", "memorytest_rnd_reps", "memorytest_chip_rnd_reversed", "memorytest_chip_walkingOnes", "memorytest_ones_reps", "memorytest_chip_walkingOnes_reversed", "memorytest_chip_burstwrite", "memorytest_burst_reps", "memorytest_rnd_seed", "memorytest_address_reps", "memorytest_chip_address_reversed", "memorytest_custom_pattern", "memorytest_chip_bitfade", "memorytest_bitfade_reps", "memorytest_bitfade_timeout", "memorytest_chip_bitfade_allZeros"];
		for(let i = 0; i < inputIds.length; i++) {
			document.getElementById(inputIds[i]).disabled = false;
		}
	}

	const disableMemoryTestSection = ()=> {
		let inputIds = ["memorytest_dataBus", "memorytest_dataBus_reps", "memorytest_addressBus", "memorytest_chip_hammer", "memorytest_hammer_reps", "memorytest_addrBus_reps", "memorytest_chip_rnd", "memorytest_chip_address", "memorytest_rnd_reps", "memorytest_chip_rnd_reversed", "memorytest_chip_walkingOnes", "memorytest_ones_reps", "memorytest_chip_walkingOnes_reversed", "memorytest_chip_burstwrite", "memorytest_burst_reps", "memorytest_rnd_seed", "memorytest_address_reps", "memorytest_chip_address_reversed", "memorytest_custom_pattern", "memorytest_chip_bitfade", "memorytest_bitfade_reps", "memorytest_bitfade_timeout", "memorytest_chip_bitfade_allZeros"];
		for(let i = 0; i < inputIds.length; i++) {
			document.getElementById(inputIds[i]).disabled = true;
		}
	}



	let databusReps = 1;
	document.getElementById('memorytest_dataBus').addEventListener('click', async() => {
		await execute_memoryTest(CMD_MEMORYTEST_DATABUS, "data bus memory test", databusReps);
	});

	let addrbusReps = 1;
	document.getElementById('memorytest_addressBus').addEventListener('click', async() => {
		await execute_memoryTest(CMD_MEMORYTEST_ADDRBUS, "address bus memory test", addrbusReps);
	});

	let chiprndreps = 1;
    document.getElementById('memorytest_chip_rnd').addEventListener('click', async() => {
  		let seedRaw = document.getElementById('memorytest_rnd_seed').value
  		let seed = parseInt(seedRaw);
  		if(isNaN(seed) || !seed) {
  			alert("The given seed '"+seedRaw+"' is not valid.");
  			setStatus("The given seed '"+seedRaw+"' is not valid.");
  			return;
  		}

  		let reqArg = format_uint32_to_reqString(seed);
  		console.log(reqArg);
  		if(document.getElementById("memorytest_chip_rnd_reversed").checked) {
  			await execute_memoryTest(CMD_MEMORYTEST_RND_REV, "memory chip test w. random & reversed pattern (Seed: '"+seedRaw+"')", chiprndreps, reqArg);
  		} else {
  			await execute_memoryTest(CMD_MEMORYTEST_RND, "memory chip test w. random pattern (Seed: '"+seedRaw+"')", chiprndreps, reqArg);
  		}
    });

    let chipFadeReps = 1;
    document.getElementById('memorytest_chip_bitfade').addEventListener('click', async() => {
  		let timeoutRaw = document.getElementById('memorytest_bitfade_timeout').value
  		let timeoutT = parseInt(timeoutRaw);
  		if(isNaN(timeoutT) || !timeoutT) {
  			alert("The given timeout time '"+timeoutRaw+"' is not valid.");
  			setStatus("The given timeout time '"+timeoutRaw+"' is not valid.");
  			return;
  		}

  		let reqArg = format_uint32_to_reqString(timeoutT*1000); // Convert the time from seconds to milliseconds
  		console.log(reqArg);
  		if(document.getElementById("memorytest_chip_bitfade_allZeros").checked) {
  			await execute_memoryTest(CMD_MEMORYTEST_BITFADE_ALLZEROS, "Bit Fade Test with all zeros (Timeout: "+timeoutRaw+" seconds)", chipFadeReps, reqArg, timeoutT);
  		} else {
  			await execute_memoryTest(CMD_MEMORYTEST_BITFADE, "Bit Fade Test with all ones (Timeout: "+timeoutRaw+" seconds)", chipFadeReps, reqArg, timeoutT);
  		}
    });
	
	let chiponesreps = 1;
    document.getElementById('memorytest_chip_walkingOnes').addEventListener('click', async() => {
  		if(document.getElementById("memorytest_chip_walkingOnes_reversed").checked) {
  			await execute_memoryTest(CMD_MEMORYTEST_ONES_REV, "memory chip test w. walking ones & reversed pattern", chiponesreps);
  		} else {
  			await execute_memoryTest(CMD_MEMORYTEST_ONES, "memory chip test w. walking ones pattern", chiponesreps);
  		}
    });

    let chipaddresssreps = 1;
    document.getElementById('memorytest_chip_address').addEventListener('click', async() => {
  		if(document.getElementById("memorytest_chip_address_reversed").checked) {
  			await execute_memoryTest(CMD_MEMORYTEST_ADDRESS_REV, "memory chip test w. address & reversed pattern", chipaddresssreps);
  		} else {
  			await execute_memoryTest(CMD_MEMORYTEST_ADDRESS, "memory chip test w. address pattern", chipaddresssreps);
  		}
    });


    let chipburstsreps = 1;
    document.getElementById('memorytest_chip_burstwrite').addEventListener('click', async() => {
    	if(document.getElementById("toggleCache").checked) {
			await execute_memoryTest(CMD_MEMORYTEST_BURSTWRITE, "Memory chip burst write test", chipburstsreps);
    	} else {
    		alert("Burst write test cannot be run while cache is disabled.");
    		setStatus("Unable to run burst write test. Enable cache in order to run this test.");
    	}

    });

    let chiphammerreps = 1;
    document.getElementById('memorytest_chip_hammer').addEventListener('click', async() => {
    	if(!document.getElementById("toggleCache").checked) {
			await execute_memoryTest(CMD_MEMORYTEST_HAMMER, "Memory chip Hammer Test", chiphammerreps);
    	} else {
    		alert("Hammer testest cannot be run while cache is enabled.");
    		setStatus("Unable to run the hammer test. Disable the cache in order to run this test.");
    	}

    });
    

    
    async function execute_memoryTest(TESTID, TESTNAME, REPS, argument, extraTime) {
		let s = disableButtons("bl2u", true);
		let counter;
		let repetitionText;
		let timeRepBefore, diffMs, diffMins, diffSecs

		extraTime = (typeof extraTime === "number") ? extraTime : 0; // If the extra time is not given, simply add zero. Used for BitFade

		// Start countdown timer
		Countdown_Prepare();
		if(document.getElementById("toggleCache").checked) {
			Countdown_Start(countdown_times.w_cache[TESTID]*REPS+extraTime);
		} else {
			Countdown_Start(countdown_times.wo_cache[TESTID]*REPS+extraTime);
		}
		
		try {
			setStatus("Executing "+ TESTNAME);

			let timeBefore = new Date();
			

			for(let i = 0; i < REPS; i++) {
				counter = 1;
				timeRepBefore = new Date(); 
				repetitionText = (REPS == 1) ? "" : (" (" + (i+1) + "/" + REPS + ") ");

				let cont = await completeRequest(port, fmtReq(TESTID, 0));
				console.log(cont);
				// If the command was acknowledged and an argument is given, send the argument
				// Used originally for the seeding part of the random pattern
				if(cont.command == 'a' && argument) {
					cont = await completeRequest(port, argument);
					console.log(cont);
				}
				
				while(cont.length == 0 || cont.data.indexOf("debug:") > -1) {
					// Get response from port_reader stream
					cont = await readRequest();
					console.log(cont);

					
					if(cont.length > 0 && cont.data.indexOf("debug:") > -1) {
						console.log("debug data: " + cont.data);
					} else {
						setStatus("Executing " + TESTNAME + " - "+(counter++)+"% done."+repetitionText, true);
					}
				}
				diffMs = (new Date()) - timeRepBefore;
				diffMins = Math.floor(diffMs / 60000).toString().padStart(2,'0'); 
				diffSecs = Math.floor((diffMs % 60000) / 1000).toString().padStart(2,'0'); 
				setStatus("Finished " + TESTNAME + repetitionText + " - Result: " + cont.data + " - Time: " + diffMins + ":"+diffSecs);
				
			}

			let timeAfter = new Date();

			let chachedEnabled = document.getElementById("toggleCache").checked;
			let cacheName = (chachedEnabled ? "_withcache" : "");
			if(!timeObject[TESTID+cacheName]) {
				timeObject[TESTID+cacheName] = {
					total_time: 0,
					total_reps: 0
				};
			}
			timeObject[TESTID+cacheName].total_time += timeAfter.getTime() - timeBefore.getTime();
			timeObject[TESTID+cacheName].total_reps += REPS;
			
		} catch(e) {
			setStatus("Memory test encountered an error: " + e);
		} finally {
			Countdown_Stop();
			restoreButtons(s);
		}
    }

	document.getElementById("memorytest_dataBus_reps").addEventListener("focusout", (e) => (databusReps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_addrBus_reps").addEventListener("focusout", (e) => (addrbusReps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_rnd_reps").addEventListener("focusout", (e) => (chiprndreps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_bitfade_reps").addEventListener("focusout", (e) => (chipFadeReps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_ones_reps").addEventListener("focusout", (e) => (chiponesreps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_address_reps").addEventListener("focusout", (e) => (chipaddresssreps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_burst_reps").addEventListener("focusout", (e) => (chipburstsreps = formatRepetitionInputField(e)));
	document.getElementById("memorytest_hammer_reps").addEventListener("focusout", (e) => (chiphammerreps = formatRepetitionInputField(e)));

	function formatRepetitionInputField(event) {
		let parsedValue = parseInt(event.target.value);
		if(!parsedValue) {
			event.target.value = 1;
		} else {
			if(parsedValue < 0) {
				event.target.value = 1;
			} else if(parsedValue > 999) {
				event.target.value = 999;
			} else {
				event.target.value = parsedValue;
			}
		}
		return event.target.value;
	}

	document.getElementById("memorytest_dataBus_reps").addEventListener("keydown", handleKeypressEventForInput);
	document.getElementById("memorytest_addrBus_reps").addEventListener("keydown", handleKeypressEventForInput);
	document.getElementById("memorytest_rnd_reps").addEventListener("keydown", handleKeypressEventForInput);
	document.getElementById("memorytest_ones_reps").addEventListener("keydown", handleKeypressEventForInput);
	function handleKeypressEventForInput(event) {
		let parsedValue = parseInt(event.target.value);
		if((event.keyCode == 40 || event.keyCode == 38) && !parsedValue) {
			event.target.value = 1;
			return event.target.value;
		}

		if(event.keyCode == 40) { // Key down arrow
			event.target.value = (parsedValue == 1) ? 1 : (parsedValue-1);
		} else if(event.keyCode == 38) { // Key up arrow
			event.target.value = (parsedValue == 999) ? 999 : (parsedValue+1);
		}
		return event.target.value;
	}

    document.getElementById('bl1_download').addEventListener('click', async () => {
		let s = disableButtons("bl1", true);
		try {
		    setStatus("Downloading BL2U applet");
		    await downloadApp(port, app, false);
		    await delaySkipInput(port, 2000);
		    let auth = await completeRequest(port, fmtReq(CMD_AUTH, 0));
		    setStatus("BL2U booting");
		    // Allow BL2U to boot
		    await delaySkipInput(port, 2000);
		    let fwu_vers = await completeRequest(port, fmtReq(CMD_VERS, 0));
		    setStatus("BL2U operational: " + fwu_vers["data"]);

		    setStage("bl2u");
		} catch(e) {
		    setStatus(e);
		} finally {
		    restoreButtons(s);

			// Disable buttons for memory testing - they're reenabled when the DDR memory is setup in function enableMemoryTestSection
			disableMemoryTestSection();
		}
    });

    document.getElementById('bl1_sjtag_challenge').addEventListener('click', async () => {
	let s = disableButtons("bl1", true);
	try {
	    var rspStruct = await completeRequest(port, fmtReq(CMD_SJTAG_RD, 0));
	    sjtag_challenge = rspStruct["data"];
	    var data = rspStruct["data"].split('').map(function (ch) {
		return ch.charCodeAt(0).toString(16).padStart(2, "0");
	    }).join(":");
	    setStatus("SJTAG Challenge read");
	    setOpStatus("bl1_sjtag_challenge_feedback", "Challenge received");
	    addTrace("SJTAG Challenge: " + data);
	    s.set('bl1_sjtag_unlock', false);
	} catch(e) {
	    setStatus("SJTAG Challenge: " + e);
	    setOpStatus("bl1_sjtag_challenge_feedback", "Failure: SJTAG is most likely not enabled in OTP");
	} finally {
	    restoreButtons(s);
	}
    });

    document.getElementById('bl1_sjtag_unlock').addEventListener('click', async () => {
	let s = disableButtons("bl1", true);
	try {
	    var key_inp = document.getElementById('bl1_sjtag_unlock_key');
	    var data;
	    try { data = hexString2Bin(key_inp.value); } catch (e) { data = ""; }
	    if (data.length != 32) {
		setOpStatus("bl1_sjtag_unlock_feedback", "Key invalid, please enter 32 bytes");
		key_inp.focus();
	    } else {
		// Now derive the key
		data = sha256(sjtag_challenge + data);
		// Go ahead and unlock
		var rspStruct = await completeRequest(port, fmtReq(CMD_SJTAG_WR, 0, data, false));
		setStatus("SJTAG Unlocked");
		setOpStatus("bl1_sjtag_unlock_feedback", "SJTAG Unlocked");
		setOpStatus("bl1_sjtag_challenge_feedback", "");
		s.set('bl1_sjtag_unlock', true);
		s.set('bl1_sjtag_challenge', true);
	    }
	} catch(e) {
	    setStatus("SJTAG Unlock: " + e);
	    setOpStatus("bl1_sjtag_unlock_feedback", "Failure: SJTAG did not unlock, key probably wrong");
	} finally {
	    restoreButtons(s);
	}
    });

    document.getElementById('bl1_otp_set_rnd').addEventListener('click', async () => {
	await doOtpRandom(port, 'bl1', 'bl1_otp_set_rnd_feedback', 'bl1_otp_set_rnd_fld');
    });

    document.getElementById('bl1_otp_set_data').addEventListener('click', async () => {
	await doOtpSetData(port, 'bl1', 'bl1_otp_set_data_feedback', 'bl1_otp_set_data_fld', 'bl1_otp_set_data_buf');
    });

    document.getElementById('bl1_continue').addEventListener('click', async () => {
	let s = disableButtons("bl1", true);
	try {
	    setStatus("Continue TFA boot");
	    var rspStruct = await completeRequest(port, fmtReq(CMD_CONT, 0));
	    setStatus("System Booting");
	    setStage("booting");
	} catch(e) {
	    setStatus("System Boot failed: " + e);
	} finally {
	    restoreButtons(s);
	}
	// Flush port for any pending boot massages
	await delaySkipInput(port, 2000);
    });

    document.getElementById('port_select').addEventListener('click', async () => {
	port = await navigator.serial.requestPort();
	port_reader = await portOpen(port);
	setStatus("Connected");
	// Avoid reconnect
	document.getElementById('port_select').disabled = true;
	setStage("none");
	// Flush port for any pending boot massages
	await delaySkipInput(port, 200);

	try {
	    setStatus("Identify platform...");
	    var rspStruct = await completeRequest(port, fmtReq(CMD_VERS, 0));
	    var plf = getKeyByValue(platforms, rspStruct["data"]);
	    if (plf) {
		setStatus("Identified device: " + plf);
		addTrace("Please select a BL1 command - or upload BL2U for firmware update functions");
		setStage("bl1");
	    } else {
		setStatus("(Assume) BL2U operational");
		addTrace("Unidentified device");
		addTrace("Device: " + rspStruct["data"] + "\n" +
			 "- which is not identified as a known boot ROM.\n" +
			 "It is assumed that the active software is a BL2U version");
		
		setStage("bl2u");

		// Disable buttons for memory testing - they're reenabled when the DDR memory is setup in function enableMemoryTestSection
		// TODO: If it has reached this state BL2U was loaded in another session - add way to check whether memory is already setup or not
		disableMemoryTestSection();
	    }
	} catch (e) {
	    console.log("Connect failed: " + e);
	}
    });

    document.getElementById('settings').addEventListener('click', async (event) => {
	if (settings_prev_stage) {
	    setStage(settings_prev_stage);
	    settings_prev_stage = undefined;
	} else {
	    settings_prev_stage = cur_stage;
	    setStage("adv_settings");
	}
    });

    document.getElementById('settings_back').addEventListener('click', async (event) => {
	if (settings_prev_stage) {
	    document.getElementById('settings').click();
	} else {
	    setStage("connect");
	}
    });

    document.getElementById('enable_trace').addEventListener('change', async (event) => {
	var inp = event.srcElement;
	tracing = inp.checked;
	console.log("trace setting: %o", tracing);
    });


	document.getElementById("memory_config_setup_default_config").addEventListener('click', async ()=>{		
		let s = disableButtons("bl2u", true);
		
		try {
			setStatus("Reading out DDR configuration");

			let cont = await completeRequest(port, fmtReq(CMD_MEMORYCONFIG_READ, 0));
			let ddrConfigObj = DDRarrayToConfig(cont.data);
			loadDDRconfigToDOM(ddrConfigObj);
		
			setStatus("Finished Reading Out Config");

			restoreButtons(s);
			
		} catch(e) {
			setStatus("Memory Configuration Readout encountered an error: " + e);
			restoreButtons(s);
		}


	});

	document.getElementById("memory_config_setup_choose_config").addEventListener('click', ()=>{
		document.getElementById("memory_config_setup_choose_config_input").click();
	});

	document.getElementById("memory_config_setup_choose_config_input").addEventListener('change', (e)=>{
		console.log(e.target.files);
		console.log(e.target.files[0]);

		if(!e.target.files[0]) {
			return;
		}

		var reader = new FileReader();
		// Set OnLoad callback
		reader.onload = function(e){
			let loadedObj = jsyaml.load(e.target.result);

			// Begin to load object!
			loadDDRconfigToDOM(loadedObj);
			globalVar = loadedObj;
		};	
		// Read file into memory as UTF-16
		reader.readAsText(e.target.files[0], "utf8");
	})

	function loadDDRconfigToDOM(ddrConfig) {
		if(typeof ddrConfig !== "object") return false; // If it is not a valid object, return false

		if(!checkOrSetDDRconfig(ddrConfig, true)){
			alert("Config file not valid!");
			return;
		}

		checkOrSetDDRconfig(ddrConfig, false);

		document.getElementById("config_field_define_info_Name").value = ddrConfig.info["version"];
		document.getElementById("config_field_define_info_Speed").value = "0x"+(ddrConfig.info["speed"].toString(16).toUpperCase()).padStart(8,"0");

		if(ddrConfig.info["mem_size_mb"]) {
			document.getElementById("config_field_define_info_Size").value = "0x"+(parseInt(ddrConfig.info["mem_size_mb"])*1024*1024).toString(16).toUpperCase().padStart(8,"0"); // Convert to bytes from mega bytes
		} else {
			document.getElementById("config_field_define_info_Size").value = "0x"+(ddrConfig.info["size"]).toString(16).toUpperCase().padStart(8,"0"); // Convert to hex
		}

		
		document.getElementById("config_field_define_info_Bus_Width").value = "0x"+(ddrConfig.info["bus_width"].toString(16).toUpperCase()).padStart(8,"0");
		return true;
	}

	// Function to recursively check object and its children - Check == true => Only check field are valid and otherwise set the field in GUI
	function checkOrSetDDRconfig(config, check) {
		for(var key in config) {
			if(key === 'info') continue; // Information field should be handled on its own
			if(typeof config[key] === 'object') {
				// Recursively check children
				if(!checkOrSetDDRconfig(config[key], check)){
					return false; // If one children fail, return false otherwise keep checking!
				}
			} else if(!document.getElementById(key)) {
				return false; // If the key is not a field found in the GUI there is a mismatch - abort and return false
			} else if(!check) {
				document.getElementById(key).value = "0x"+(config[key].toString(16).toUpperCase()).padStart(8,"0");
			}
		}
		return true;
	}


	document.getElementById("memory_config_setup_btn").addEventListener('click', async() => {
		let s = disableButtons("bl2u", true);
		
		try {
			setStatus("Initializing the DDR Memory with Custom parameters");

			let success = GetDDRconfigFromDOM(); // Create the object from the DOM
			if(!success) {
				throw "Given config is not valid!";
			}

			let cont = await completeRequest(port, fmtReq(CMD_MEMORYCONFIG_INIT_CUSTOM, 0));
			console.log(cont);

			let req = format_ddr_config_to_data_request(ddr_config_interface); // Pass this object to the formatter
			console.log(req);
			cont = await completeRequest(port, req);

			console.log(cont);
			cont = await readRequest();
			setStatus("Finished Initializing Custom Memory Config - Result: " + cont.data);

			restoreButtons(s);
			enableMemoryTestSection();
			
		} catch(e) {
			setStatus("Memory configuration encountered an error: " + e);
			restoreButtons(s);
		} 
	});

	function GetDDRconfigFromDOM() {
		// ddr_config_interface
		ddr_config_interface.info.version = document.getElementById('config_field_define_info_Name').value;
		ddr_config_interface.info.speed = document.getElementById('config_field_define_info_Speed').value;
		ddr_config_interface.info.size = document.getElementById('config_field_define_info_Size').value;
		ddr_config_interface.info.bus_width = document.getElementById('config_field_define_info_Bus_Width').value;		

		// Recursively get all values - input field have the same ID as the keys - returns true if it was succesfull
		return (function(obj){
			for(var key in obj) {
				if(key === "info") continue; // Info fiels are set manually

				if(typeof obj[key] === "object") {
					if(!arguments.callee(obj[key])) { // Recursively find children
						return false; // If some child false, abort this and return false
					}
				} else {
					let parsedValue = parseInt(document.getElementById(key).value);
					if(!parsedValue && parsedValue != 0) {
						alert(key + " is not formatted correctly! Got: " + document.getElementById(key).value);
						return false;
					}

					obj[key] = parsedValue;
				}
			}
			return true;

		})(ddr_config_interface);
	}


	for(var elem of document.getElementsByClassName("config_field_selector")) {

		elem.addEventListener('click', (e)=>{
			// Remove visibility of previosly shown section
			for(var elem of document.getElementsByClassName("config_field_define_area_clicked")) {
				elem.classList.remove("config_field_define_area_clicked");
			}
			// Remove clicked color of previosly clicked tabs
			for(var elem of document.getElementsByClassName("config_field_selector_clicked")) {
				elem.classList.remove("config_field_selector_clicked");
			}

			// Add clicked styling to the clicked tab
			e.target.classList.add("config_field_selector_clicked");

			// Add visibility for clicked element
			switch(e.target.innerText) {
				case 'Info':
					document.getElementById("config_field_define_info").classList.add("config_field_define_area_clicked");
					break;
				case 'Main':
					document.getElementById("config_field_define_main").classList.add("config_field_define_area_clicked");
					break;
				case 'Timing':
					document.getElementById("config_field_define_timing").classList.add("config_field_define_area_clicked");
					break;
				case 'Mapping':
					document.getElementById("config_field_define_mapping").classList.add("config_field_define_area_clicked");
					break;
				case 'Phy':
					document.getElementById("config_field_define_phy").classList.add("config_field_define_area_clicked");
					break;
				case 'Phy-Timing':
					document.getElementById("config_field_define_phy_timing").classList.add("config_field_define_area_clicked");
					break;
				default:
					break;
				
			}
		})
	}

	document.getElementById("toggleCache").addEventListener('change', async(e) => {
		let enableCache = e.target.checked;
		let s = disableButtons("bl2u", true);

		try {
			setStatus(enableCache ? "Enabling Cache" : "Disabling Cache");

			let cont = await completeRequest(port, fmtReq((enableCache ? CMD_ENABLE_CACHE : CMD_DISABLE_CACHE), 0));
			// console.log(cont);
			// cont = await readRequest();
			setStatus(cont.data);

			restoreButtons(s);
		} catch(e) {
			setStatus("Toggling Cache Encountered an Error: " + e);
			restoreButtons(s);
		} 
	});
		
	// Attach click event to Upload Custom Pattern Code button
	document.getElementById("memorytest_custom_pattern").addEventListener('click', async ()=>{

	    let s = disableButtons("bl2u", true);
		
		try {
			setStatus("Testing with custom pattern");

			let code = document.getElementById("memorytest_custom_pattern_input").value;
		    console.log(code);
		    let instructions = assemble_program(code);
		    console.log(instructions);

			let cont = await completeRequest(port, fmtReq(CMD_MEMORYTEST_CUSTOM, 0));
			console.log(cont);

			let req = format_customPattern_to_hexString(instructions); 
			console.log(req);
			cont = await completeRequest(port, req);

			console.log(cont);
			cont = await readRequest();
			setStatus("Finished memory testing with custom pattern - Result: " + cont.data);

			restoreButtons(s);
			enableMemoryTestSection();
			
		} catch(e) {
			setStatus("Memory testing with custom pattern encountered an error: " + e);
			restoreButtons(s);
		} 
	});


	// Implement simple countdown functionality
	let run_timer = false;
	let time_counter;
	// Prepares the timer - simply used to avoid race conditions
	function Countdown_Prepare() {
		run_timer = true;
	}
	function Countdown_Reset() {
		time_counter = 0;
	}
	// Start a timer given an expected time given as seconds
	async function Countdown_Start(time_s) {
		time_s = Math.ceil(time_s); // Operates on integers
		if(time_s <= 2) 
			return; // Dont bother showing the countdown if the time is 2 or less seconds to finish

		document.getElementById("countdown_timer").style.display = '';

		Countdown_Reset();
		while(run_timer) {
			let exp_time_left = Math.abs(time_s-time_counter);
			let mins = (Math.floor(exp_time_left/60)).toString().padStart(2,"0");
			let secs = (exp_time_left % 60).toString().padStart(2,"0");
			if(time_counter > time_s) {
				document.getElementById("countdown_timer_time").innerText = "-" + mins + ":" + secs;
			} else {
				document.getElementById("countdown_timer_time").innerText = mins + ":" + secs;
			}
			time_counter++;
			await sleep(1000);
		}
	}
	// Stop the timer
	function Countdown_Stop() {
		run_timer = false;
		document.getElementById("countdown_timer").style.display = 'none';
	}
	// Utility function to use sleep functionality. Source: https://stackoverflow.com/questions/951021/what-is-the-javascript-version-of-sleep
	function sleep(ms) {
	    return new Promise(resolve => setTimeout(resolve, ms));
	}
}
