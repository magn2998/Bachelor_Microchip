let opands = {// id  type
    "stop":      [0,  1],
    "repeat":    [1,  2],
    "end":       [2,  1],
    "store":     [3,  3],
    "load":      [4,  3],
    "cmp":       [5,  3],
    "mov":       [6,  3],
    "movi":      [7,  4],
    "movetop":   [8,  4],
    "add":       [9,  5],
    "addi":      [10, 6],
    "sub":       [11, 5],
    "subi":      [12, 6],
    "and":       [13, 5],
    "andi":      [14, 6],
    "or":        [15, 5],
    "ori":       [16, 6],
    "xor":       [17, 5],
    "xori":      [18, 6],
    "lsl":       [19, 5],
    "lsli":      [20, 6],
    "lsr":       [21, 5],
    "lsri":      [22, 6],
    "mul":       [23, 5],
    "muli":      [24, 6],
    "negate":    [25, 3],
    "repeatequals": [26,  3]
};

// Registers = m (maxAddress - constant), p (pattern), n (address), r1 - r5 (Temp registers)
let registerLookup = {
    "max": 0,
    "m":   0,
    "p":  1,
    "n":  2,
    "r1": 3,
    "r2": 4,
    "r3": 5,
    "r4": 6,
    "r5": 7
};

/*
types:
    1. cmd (e.g. 'loop' or 'restart')
    2. cmd + imm (e.g. 'repeat')
    3. cmd + rd + r1 (e.g. 'CMP' or 'negate')
    4. cmd + rd + imm (e.g. 'movi')
    5. cmd + rd + r1 + r2 (e.g. 'add')
    6. cmd + rd + r1 + imm (e.g. 'addi')
*/

function assemble_program(str) {        // Replace Comments and semicolons
    let prog = str.toLowerCase().replace(/\/\/.*\n|:/g, '\n').split(/ *\n+ */g).filter((e)=>(e.indexOf('start') === -1 && e != '')).map((e)=>e.split(/[, ]+/g));
    let instructions = new Array(64).fill(0);

    if(prog.length > 64) {
        throw "Program too large! Must be a maximum of 64 instructions";
    }

    for(let i = 0; i < prog.length; i++) {
        let cmd = prog[i];
        let operand = opands[cmd[0]];
        if(operand === undefined) 
            throw "Command " + cmd + " doesn't exist."

        instructions[i] = assemble_command(cmd, operand);
    }

    return instructions;
}

function assemble_command(cmd, operand) { // Given as an array of 'tokens'
    switch(operand[1]) { // Check command type
        case 1:
            return assemble_command_1(cmd, operand);
            break;
        case 2:
            return assemble_command_2(cmd, operand);
            break;
        case 3:
            return assemble_command_3(cmd, operand);
            break;
        case 4:
            return assemble_command_4(cmd, operand);
            break;
        case 5:
            return assemble_command_5(cmd, operand);
            break;
        case 6:
            return assemble_command_6(cmd, operand);
            break;
        default:
            throw "Catastrophic Error"
    }
}

//cmd (e.g. 'loop' or 'restart')
function assemble_command_1(cmd, operand) { 
    if(cmd.length !== 1)
        throw "command '" + cmd + "' is formatted wrongly";
    return operand[0]; // Instruction is just the command ID
}

//cmd + imm (e.g. 'repeat')
function assemble_command_2(cmd, operand) { 
    if(cmd.length !== 2)
        throw "command '" + cmd + "' is formatted wrongly";
    if(cmd[1][0] !== "#")
        throw "command '" + cmd + "' expects a value beginning with '#'";
    let parsedVal = parseInt(cmd[1].substring(1));
    if(parsedVal === NaN)
        throw "command '" + cmd + "' has wrongly formatted value";
    return operand[0] | (parsedVal << 15);
}

//cmd + rd + r1 (e.g. 'CMP' or 'negate')
function assemble_command_3(cmd, operand) { 
    if(cmd.length !== 3)
        throw "command '" + cmd + "' is formatted wrongly";
    let rd = registerLookup[cmd[1]];
    let r1 = registerLookup[cmd[2]];

    if(rd === undefined || r1 === undefined) {
        throw "Registers for " + cmd + " not available."
    }

    return operand[0] | ((rd & 0x7) << 6) | ((r1 & 0x7) << 9);
}

//cmd + rd + imm (e.g. 'movi')
function assemble_command_4(cmd, operand) { 
    if(cmd.length !== 3)
        throw "command '" + cmd + "' is formatted wrongly";
    let rd = registerLookup[cmd[1]];

    if(rd === undefined) {
        throw "Registers for " + cmd + " not available."
    }

    if(cmd[2][0] !== "#")
        throw "command '" + cmd + "' expects a value beginning with '#'";
    let parsedVal = parseInt(cmd[2].substring(1));
    if(parsedVal === NaN)
        throw "command '" + cmd + "' has wrongly formatted value";

    return operand[0] | ((rd & 0x7) << 6) | (parsedVal << 15);
}

//cmd + rd + r1 + r2 (e.g. 'add')
function assemble_command_5(cmd, operand) { 
    if(cmd.length !== 4)
        throw "command '" + cmd + "' is formatted wrongly";
    let rd = registerLookup[cmd[1]];
    let r1 = registerLookup[cmd[2]];
    let r2 = registerLookup[cmd[3]];

    if(rd === undefined || r1 === undefined || r2 === undefined) {
        throw "Registers for " + cmd + " not available."
    }

    return operand[0] | ((rd & 0x7) << 6) | ((r1 & 0x7) << 9) | ((r2 & 0x7) << 12);
}

//cmd + rd + r1 + imm (e.g. 'addi')
function assemble_command_6(cmd, operand) { 
    if(cmd.length !== 4)
        throw "command '" + cmd + "' is formatted wrongly";
    let rd = registerLookup[cmd[1]];
    let r1 = registerLookup[cmd[2]];

    if(rd === undefined || r1 === undefined) {
        throw "Registers for " + cmd + " not available."
    }

    if(cmd[3][0] !== "#")
        throw "command '" + cmd + "' expects a value beginning with '#'";
    let parsedVal = parseInt(cmd[3].substring(1));
    if(isNaN(parsedVal))
        throw "command '" + cmd + "' has wrongly formatted value";

    return operand[0] | ((rd & 0x7) << 6) | ((r1 & 0x7) << 9) | (parsedVal << 15);
}