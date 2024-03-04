.data
_prompt: .asciiz "Enter an integer:"
_ret: .asciiz "\n"
.globl main
.text
read:
  li $v0, 4
  la $a0, _prompt
  syscall
  li $v0, 5
  syscall
  jr $ra

write:
  li $v0, 1
  syscall
  li $v0, 4
  la $a0, _ret
  syscall
  move $v0, $0
  jr $ra

hanoi:
  move $fp, $sp
  lw $t0, 8($fp)
  li $t1, 1
  beq $t0, $t1, label1
  j label2
  label1:
  addi $sp, $sp, -4
  lw $t1, 12($fp)
  li $t2, 1000000
  mul $t0, $t1, $t2
  sw $t0, -4($fp)
  addi $sp, $sp, -4
  lw $t1, -4($fp)
  lw $t2, 20($fp)
  add $t0, $t1, $t2
  sw $t0, -8($fp)
  lw $t0, -8($fp)
  move $a0, $t0
  addi $sp, $sp, -4
  sw $ra, 0($sp)
  jal write
  lw $ra, 0($sp)
  addi $sp, $sp, 4
  j label3
  label2:
  addi $sp, $sp, -8
  addi $sp, $sp, -4
  lw $t1, 8($fp)
  li $t2, 1
  sub $t0, $t1, $t2
  sw $t0, -12($fp)
  lw $t0, 16($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  lw $t0, 20($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  lw $t0, 12($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  lw $t0, -12($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  addi $sp, $sp, -8
  sw $fp, 0($sp)
  sw $ra, 4($sp)
  jal hanoi
  move $sp, $fp
  lw $ra, 4($sp)
  lw $fp, 0($sp)
  addi $sp, $sp, 24
  addi $sp, $sp, -4
  move $t0, $v0
  sw $t0, -16($fp)
  label3:
  addi $sp, $sp, -8
  li $t0, 0
  move $v0, $t0
  jr $ra
main:
  move $fp, $sp
  addi $sp, $sp, -4
  li $t1, 3
  move $t0, $t1
  sw $t0, -4($fp)
  li $t0, 3
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  li $t0, 2
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  li $t0, 1
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  lw $t0, -4($fp)
  addi $sp, $sp, -4
  sw $t0, 0($sp)
  addi $sp, $sp, -8
  sw $fp, 0($sp)
  sw $ra, 4($sp)
  jal hanoi
  move $sp, $fp
  lw $ra, 4($sp)
  lw $fp, 0($sp)
  addi $sp, $sp, 24
  addi $sp, $sp, -4
  move $t0, $v0
  sw $t0, -8($fp)
  li $t0, 0
  move $v0, $t0
  jr $ra
