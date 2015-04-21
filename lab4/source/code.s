.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text
#FUNCTION fact : 
_fact:
addi $sp, $sp, -2048

#PARAM v1 
lw $t1 8($fp)
sw $t1 -4($fp)

#IF v1 != #1 GOTO label1 
lw $t2 -4($fp)
li $t3 1
bne $t2, $t3, Label1

#RETURN v1 
addi $sp, $sp, 2048
lw $v0 -4($fp)
jr $ra
#GOTO label2 
j Label2

#LABEL label1 : 
Label1:

#t5 := v1 
lw $t1 -4($fp)
sw $t1 -8($fp)

#t7 := v1 - #1 
lw $t2 -4($fp)
li $t3 1
sub $t1, $t2, $t3
sw $t1 -12($fp)

#ARG t7 

#t6 := CALL fact 
lw $t1 -12($fp)
sw $t1 -16($fp)
sw $fp -20($fp)
sw $ra -24($fp)
addi $fp, $fp, -24
jal _fact
lw $ra 0($fp)
lw $fp 4($fp)
move $t1, $v0
sw $t1 -16($fp)

#t4 := t5 * t6 
lw $t2 -8($fp)
lw $t3 -16($fp)
mul $t1, $t2, $t3
sw $t1 -20($fp)

#RETURN t4 
addi $sp, $sp, 2048
lw $v0 -20($fp)
jr $ra
#LABEL label2 : 
Label2:

#FUNCTION main : 
main:
move $fp $sp
addi $sp, $sp, -2048

#READ t10 
li $v0, 4
la $a0, _prompt
syscall
li $v0, 5
syscall
move $t1, $v0
sw $t1 -4($fp)

#v2 := t10 
lw $t1 -4($fp)
sw $t1 -8($fp)

#IF v2 <= #1 GOTO label3 
lw $t2 -8($fp)
li $t3 1
ble $t2, $t3, Label3

#ARG v2 

#t13 := CALL fact 
lw $t1 -8($fp)
sw $t1 -12($fp)
sw $fp -16($fp)
sw $ra -20($fp)
addi $fp, $fp, -20
jal _fact
lw $ra 0($fp)
lw $fp 4($fp)
move $t1, $v0
sw $t1 -12($fp)

#v3 := t13 
lw $t1 -12($fp)
sw $t1 -16($fp)

#GOTO label4 
j Label4

#LABEL label3 : 
Label3:

#v3 := #1 
li $t1, 1
sw $t1 -16($fp)

#LABEL label4 : 
Label4:

#WRITE v3 
lw $a0 -16($fp)
li $v0, 1
syscall
li $v0, 4
la $a0, _ret
syscall

#RETURN #0 
addi $sp, $sp, 2048
li $v0 0
jr $ra
