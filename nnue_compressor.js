const fs = require('fs');
const base = 256;
const quantize = 64;
const nnue_file = 'weight_NNUE3o4_64_16_8_s0.txt';
const nnue_layers = [64, 16, 8];
const cpp_out = nnue_layers[0] + "_" + nnue_layers[1] + "_" + nnue_layers[2] + "_q" + quantize + '.cpp';
const nnue_out = 'NNUE_quantized.txt';

function startDecode()
{
	h = 1; //high
	c = 0; //current
	i = 0; //index
}

function decode(p, x)
{
	while(h < 16384)
	{
		h *= base;
		c *= base;
		c += x[i++];
	}
	t = h * p >> 8;
	if(c < t)
	{
		h = t;
		return 0;
	}
	else
	{
		c -= t;
		h -= t;
		return 1;
	}
}

function saveDecode()
{
	return { "h": h, "c": c, "i": i };
}

function loadDecode(saveState)
{
	h = saveState.h;
	c = saveState.c;
	i = saveState.i;
}

function states_equal(s1, s2)
{
	return s1.h === s2.h && s1.c === s2.c && s1.i === s2.i;
}

function bisect(s1, s2)
{
	var ret = [];
	var idx = 0;
	while(true)
	{
		var c1 = idx < s1.length ? s1[idx] : 0;
		var c2 = idx < s2.length ? s2[idx] : base-1;
		idx++;
		if(c1 === c2)
		{
			ret.push(c1);
			continue;
		}

		//find anywhere we can split
		if(c2 - c1 > 1)
		{
			ret.push(Math.round((c1+c2)/2));
			return ret;
		}
		
		//if they are consecutive then split at (low, high...) or (high, low)
		if(s1.length <= s2.length)
		{
			ret.push(c1);
			ret.push(base-1);
			while(ret.length <= s1.length)
				ret.push(base-1);
			return ret;
		}
		else
		{
			ret.push(c2);
			ret.push(0);
			while(ret.length <= s2.length)
				ret.push(0);
			return ret;
		}
	}
}

function encode(bs,ps)
{
	var log = false;
	var bpIdx = 0;
	var out_low = [];
	var out_high = [];
	var out_prefix = [];
	startDecode();
	var state = saveDecode();
	
	while(true)
	{
		if(bpIdx >= bs.length)
			return out_high;
		
		loadDecode(state);
		while(out_low.length < state.i + 4)
			out_low.push(0);
		while(out_high.length < state.i + 4)
			out_high.push(base-1);
		
		loadDecode(state);
		var b_low = decode(ps[bpIdx], out_low);
		state_low = saveDecode();
		
		loadDecode(state);
		var b_high = decode(ps[bpIdx], out_high);
		state_high = saveDecode();

		if(log)
		{
			console.log("bit = " + bpIdx);
			console.log("out_low  =" + out_low);
			console.log("b_low    =" + b_low);
			console.log("out_high =" + out_high);
			console.log("b_high   =" + b_high);
		}
		
		//Are we done with this bit?
		if(b_low === b_high && states_equal(state_low, state_high))
		{
			if(log) console.log("Locked in bit " + bpIdx);
			state = state_low;
			
			//Trim known bytes
			out_prefix.push(...out_low.slice(0, state.i));
			out_low = out_low.slice(state.i);
			out_high = out_high.slice(state.i);
			state.i = 0;
			
			//Done with bit
			bpIdx++;
			// if(bpIdx % 10000 == 0)
				// console.log(bpIdx + "...");
			
			//We got all the data, we're done!
			if(bpIdx >= ps.length)
				return out_prefix;
			
			continue;			
		}
		
		if(b_low > b_high)
		{
			console.log("Something horrible went wrong");
			return [];
		}
		
		//Bisect
		var out_mid = bisect(out_low, out_high);
		while(out_mid.length < state.i + 4)
			out_mid.push(0); // whatever, as long as bisect works this shouldn't matter
		loadDecode(state);
		var b_mid = decode(ps[bpIdx], out_mid);
		state_mid = saveDecode();
		
		//Keep getting bits until we can determine where b is relative to b_mid
		var pIdx = bpIdx;
		while(b_mid === bs[pIdx])
		{
			pIdx++;
			if(pIdx >= ps.length)
			{
				//We got all the data, we're done!
				return out_prefix.concat(out_mid.slice(0, state_mid.i));
			}

			while(out_mid.length < state_mid.i + 4)
				out_mid.push(0); // whatever, as long as bisect works this shouldn't matter
			
			loadDecode(state_mid);
			b_mid = decode(ps[pIdx], out_mid);
			state_mid = saveDecode();
		}
		
		if(bs[pIdx] < b_mid)
		{
			out_high = out_mid;
		}
		else
		{
			out_low = out_mid;
		}
	}
}

function entropy(bs, ps)
{
	var cumul_entropy = 0;
	for(var i = 0; i < bs.length; i++)
		cumul_entropy += bs[i] ? Math.log(1-ps[i]) : Math.log(ps[i]);
	return cumul_entropy / Math.log(.5);
}

let data = "", a=b=c=0;
try 
{
	data = fs.readFileSync(nnue_file, 'utf8');
	[a,hidden1,hidden2] = nnue_layers;
}
catch (err)
{
	console.error(err);
}

/*
Format:
PST[768][56] - piece*64 + square - W Pawn, B Pawn, W Knight, etc
l1_bias[56]
l2_weights[56][16]
l2_bias[16]
l3_weights[16][16]
l3_bias[16]
l4_weights[16]
l4_bias[1]
*/

let sizes = [768*a, a, a*hidden1, hidden1, hidden1*hidden2, hidden2, hidden2, 1, 
	a*hidden1, hidden1, hidden1*hidden2, hidden2, hidden2, 1,
	a*hidden1, hidden1, hidden1*hidden2, hidden2, hidden2, 1,
	a*hidden1, hidden1, hidden1*hidden2, hidden2, hidden2, 1]
let chunks = [];
let chunk = [];
let i = 0;
let size_idx = 0;
for(const x of data.split("\n"))
{
	if(x.length == 0)
		continue;
	chunk.push(+x);
	i += 1;
	if(i == sizes[size_idx])
	{
		i = 0;
		size_idx += 1;
		chunks.push(chunk);
		chunk = [];
	}
}

if(i != 0)
	console.log("ERROR: i != 0", i);
if(size_idx != 8)
	console.log("ERROR: size_idx != 8");

for(const chunk of chunks)
{
	const sum = chunk.reduce((a,b) => a + Math.abs(b), 0);
	const avg = sum / chunk.length;
	console.log("length", chunk.length, "abs avg", avg, "min", Math.min(...chunk), "max", Math.max(...chunk));
}

// for(i = 128; i < 128+64; ++i)
// {
	// vector = [];
	// for(j = 0; j < 56; ++j)
		// vector.push(chunks[0][i* 56 + j]);
	// //if(i < 8 || i >= 56)
	// //	continue;
	// console.log(vector.join(","));	
// }

// return;

let all_data = [];
let all_data_uncompressed = [];
i = 0;
for(let piece = 0; piece < 6; ++piece)
	for(let color = 0; color < 2; ++color)
		for(let square = 0; square < 64; ++square)
			for(let j = 0; j < a; ++j, ++i)
			{
				if(piece == 0 && (square < 8 || square >= 56))
					continue;
				
				let quantized = Math.round(chunks[0][i]*quantize)/quantize;
				
				if(square == 0 || (square == 8 && piece == 0))
					all_data.push(quantized)
				else
					all_data.push(quantized - Math.round(chunks[0][i-a]*quantize)/quantize);
				all_data_uncompressed.push(quantized);
			}
if(i != 768 * a)
	console.log("ERROR i != 768", i);

for(i = 1; i < chunks.length; ++i)
	for(const x of chunks[i])
		all_data.push(x);
	
console.log("all data length:", all_data.length);

let buckets = Array(15).fill(0);
let signs   = Array(15).fill(0);
for(let x of all_data)
{
	let b = 0;
	y = Math.abs(Math.round(x * quantize));
	while(y)
	{
		y >>= 1;
		b++;
	}
	buckets[b]++;
	signs[b] += x < 0;
}
buckets = buckets.map(x => x / all_data.length);
signs = signs.map((x,i) => x / all_data.length / buckets[i]);
console.log("magnitude bucket probabilities:", buckets);
//console.log("sign probabilities:", signs);
let e = buckets.reduce((a,b,idx) => a + (b ? b * (idx + Math.log(b)/ Math.log(.5)) : 0), 0);
console.log("expected bits/weight:", e);
let cutoffs = [];
let p = 1;
for(const b of buckets)
{
	cutoffs.push(Math.max(1, Math.floor(b * 256/ p)));
	p -= b;
}

/*
format:
get number of Bits
0 => 0
1 => sign bit, x = 1/quantize
2 => sign bit, x = (2 + bit) / quantize
3 => sign bit, x = (4 + 2 bits) / quantize
*/

let bits = [];
let ps = [];
for(let x of all_data)
{
	if(bits.length != ps.length)
		throw "bits and ps wrong length";
	
	x = Math.round(x * quantize);
	let sign = x < 0 ? 1 : 0;
	
	y = Math.abs(x)
	let b = 0;
	while(y)
	{
		y >>= 1;
		b++;
	}
	
	for(let i = 0; i <= b; i++)
	{
		bits.push(+(i != b));
		ps.push(cutoffs[i]);
	}
	
	if(b == 0)
		continue;
	
	bits.push(sign);
	ps.push(128);
	
	x = Math.abs(x);
	for(let i = 0; i < b - 1; i++)
	{
		bits.push(x&1);
		ps.push(128);
		x >>= 1;
	}
	
	if(x != 1)
		throw "high bit not 1";
}

console.log("Bits = ", bits.length);
encoded = encode(bits, ps);
console.log("Encoded bytes = ", encoded.length);
console.log("Actual bits/weight = ", encoded.length * 8 / all_data.length);
startDecode();
let decoded = [];
while(decoded.length < all_data.length)
{
	let exp = 0;
	while(decode(cutoffs[exp], encoded))
		exp += 1;
	
	if(exp == 0)
	{
		decoded.push(0);
		continue;
	}
	
	let x = 0;
	let sign = decode(128, encoded) ? -1 : 1;
	for(let i = 0; i < exp - 1; ++i)
		x += decode(128, encoded) * 2**i;
	x += 2**(exp-1);
	decoded.push(x / quantize * sign);
}

console.log("Original = ", all_data.slice(-10));
console.log("Decompressed = ", decoded.slice(-10));

console.log("Saving quantized weights to", nnue_out);
try {
	let all_quantized = [];
	let i = 0;
	for(let piece = 0; piece < 6; ++piece)
		for(let color = 0; color < 2; ++color)
			for(let square = 0; square < 64; ++square)
				for(let j = 0; j < a; ++j)
				{
					if(piece == 0 && (square < 8 || square >= 56))
						all_quantized.push(0);
					else
						all_quantized.push(all_data_uncompressed[i++])
				}
	for(; i < all_data_uncompressed.length; ++i)
		all_quantized.push(all_data_uncompressed[i]);
	
	fs.writeFileSync(nnue_out, all_quantized.join("\n"));
} catch (err) {
	console.error(err);
}

console.log("Saving byte arrays to", cpp_out);
try {
	let out = [
		"#define INPUT_LAYER "+a,
		"\n#define HIDDEN1 "+hidden1,
		"\n#define HIDDEN2 "+hidden2,
		"\n#define QUANTIZE "+quantize,
		"\n\nuint32_t thresholds[] = {" + cutoffs.join(",") + "};\nuint8_t compressed_data[] = {"
	];
	for(let i = 0; i < encoded.length; ++i)
	{
		if(i!=0)
			out.push(",");
		if(i%20 == 0)
			out.push("\n\t");
		out.push(encoded[i].toString().padStart(3, " "));
	}
	out.push("};");
	fs.writeFileSync(cpp_out, out.join(""));
} catch (err) {
	console.error(err);
}

console.log("NNUE Check:", chunks[0][16*a]*256, chunks[0][16*a+1]*256, chunks.at(-1).at(-1)*256);

console.log("Success!");