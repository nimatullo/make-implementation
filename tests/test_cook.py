import subprocess
import argparse
import sys
import io
import signal
from subprocess import Popen, PIPE, STDOUT

WAIT_TIME = 0.02
class Task():
	def __init__(self, recipe):
		self.steps = []
		self.recipe = recipe
		self.in_file = None
		self.out_file = None
		self.message = ''
		self.piped = False
	def __str__(self):
		return str(self.recipe)
	def __repr__(self):
		return str(self.recipe)
class Recipe():
	def __init__(self):
		self.name = None
		self.this_depends_on = []
		self.depend_on_this = []
		self.tasks = []
		self.in_progress = False
	def __str__(self):
		return self.name
	
	def __repr__(self):
		return self.name

class Cookbook():
	def __init__(self):
		self.recipes = []
		self.queue = []

class Info():
	def __init__(self, is_start, start_time, pid, delay, words=''):
		self.is_start = is_start
		self.start_time = start_time
		self.pid = pid
		self.delay = delay
		self.words = words
	
	def __str__(self):
		return '{:s} {:f} {:d} {:d} {:s}'.format(('START' if self.is_start else 'END'), self.start_time, self.pid, self.delay, self.words)

	def __repr__(self):
		return '{:s} {:f} {:d} {:d} {:s}'.format(('START' if self.is_start else 'END'), self.start_time, self.pid, self.delay, self.words)

def parse_task(file, recipe):
	task_line = file.readline()
	if not task_line or task_line.strip() == '':
		return None

	task = Task(recipe)
	task_line = task_line.strip()

	if '|' in task_line:
		steps = [t for t in task_line.split('|') if t != '']
		for i in range(len(steps)):
			if i == 0 and '<' in steps[i]:
				task.in_file = steps[i][steps[i].find('<') + 1:].strip()
			if i == len(steps) - 1 and '>' in steps[i]:
				task.out_file = steps[i][steps[i].find('>') + 1:].strip()
			if '-m' in steps[i]:
				message = steps[i].find('-m')
				task.message = steps[i][message + 3:].split(' ')[0] + "\n" + task.message
			task.steps.append(steps[i].split('<')[0].split('>')[0].strip())
		task.piped = True
	else:
		in_file = task_line.find('<')
		out_file = task_line.find('>')
		message = task_line.find('-m')

		if in_file != -1:
			in_file = task_line[in_file + 1:].split('>')[0].strip()
			task.in_file = in_file
		if out_file != -1:
			out_file = task_line[out_file + 1:].split('<')[0].strip()
			task.out_file = out_file
		if message != -1:
			message = task_line[message + 3:].split(' ')[0]
			task.message = message + '\n'
		
		task.steps.append(task_line.split('<')[0].split('>')[0].strip())
	return task


def parse_recipe_header(file, recipe):
	line = file.readline()

	# Skip any prior whitespace
	while line and line.strip() == '':
		line = file.readline()
	
	line = line.strip()

	# At this point there is name and dependencies
	if ':' not in line:
		#print("Expected ':' in recipe header")
		return False
	line = line.split(':')
	name = line[0]
	dependencies = line[1] if len(line) == 2 else ''

	# Associate links with dependencies
	recipe.name = name
	dependencies = [d for d in dependencies.split(' ') if d != '']
	for d in dependencies:
		recipe.this_depends_on.append(d)
	return True

def parse_recipe(file):
	recipe = Recipe()
	if not parse_recipe_header(file, recipe):
		return None
	
	task = parse_task(file, recipe)
	while task:
		recipe.tasks.append(task)
		task = parse_task(file, recipe)
	return recipe

def get_recipe(cookbook, name):
	for recipe in cookbook.recipes:
		if recipe.name == name:
			return recipe
	return None

def set_dependencies(cookbook):
	for recipe in cookbook.recipes:

		for i, name in enumerate(recipe.this_depends_on):
			rp = get_recipe(cookbook, name)
			if not rp:
				print("Dependent recipe does not exist")
				return 1
			recipe.this_depends_on[i] = rp
			# Set dependency
			rp.depend_on_this.append(recipe)
	return 0

def print_cookbook(cookbook):
	for r in cookbook.recipes:
		print(r.name)
		for dep in r.this_depends_on:
			print('d#', dep.name)
		for task in r.tasks:
			print('-T#')
			for step in task.steps:
				print('--S#:', step)

def parse_cookbook(file):
	cb = Cookbook()
	with open(file, 'r+') as f:
		recipe = parse_recipe(f)
		while recipe:
			cb.recipes.append(recipe)
			recipe = parse_recipe(f)
	set_dependencies(cb)
	return cb

def timeout(signum, frame):
	raise Exception("")

def get_student_transcript(argv, max_recipes):
	signal.signal(signal.SIGALRM, timeout)
	signal.alarm(max_recipes * 2)
	try:
		print("Running ", ' '.join(argv))
		result = subprocess.run(argv, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
		#p = Popen(argv, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
		#stdout, stderr = p.communicate(timeout=max_recipes*2)
	except Exception as e:
		print("ERROR: Program hanging on a '", ' '.join(argv), "'", e, sep='')
		sys.exit(1)
	signal.signal(signal.SIGALRM, signal.SIG_IGN)
	print("stdout:\n", result.stdout)
	print("stderr:\n", result.stderr)
	print("rc: ", result.returncode)
        #return stderr.decode('utf8'), stdout.decode('utf8'), p.returncode
	return result.stderr.decode('utf8'), result.stdout.decode('utf8'), result.returncode

def parse_args():
	parser = argparse.ArgumentParser(description='Analyze cook book program provided', 
		usage='script.py [-p cook] [-f cookbook] [-c max_cooks] [-m main_recipe_name] [-e]')
	parser.add_argument('-p', default='bin/cook', help='path of cook program to execute (default "bin/cook")')
	parser.add_argument('-f', help='path of cookbook to process')
	parser.add_argument('-c', type=int, help='number of cooks to use')
	parser.add_argument('-e', action='store_true', help='check if error')
	parser.add_argument('-w', type=int, default=20, help='set wait threshold')
	parser.add_argument('-m', help='main recipe to use')
	parser.add_argument('-x', help='expected output')
	return parser.parse_args()

def get_leaves(root, array):
	if not root:
		return
	if len(root.this_depends_on) == 0:
		return array.add(root)

	for cur in root.this_depends_on:
		get_leaves(cur, array)


# Parse a line from the generic_step stub which is of format
# (END|START)\t[time_start, pid, delay](\n| program_name ...)
def get_stub_info(stub_line):
	first_space = stub_line.find('\t')

	right_bracket = stub_line.find(']')
	try:
		time, pid, delay = tuple(stub_line[first_space + 2: right_bracket].split(','))
	except ValueError:
		print("ERROR: Unexpected output '" + stub_line + "'")
		sys.exit(1)

	if stub_line[-1] == ']':
		return Info(False, float(time), int(pid), int(delay))
	
	return Info(True, float(time), int(pid), int(delay), stub_line[right_bracket + 1:].strip())

def remove_recipe(recipe, cookbook, ready_set):
	add_to_set = set()

	recipe.in_progress = False
	for r in recipe.depend_on_this:
		r.this_depends_on.remove(recipe)
		#print(r, len(r.this_depends_on), r.tasks[0].steps if len(r.tasks) > 0 else '')
		if len(r.this_depends_on) == 0 and len(r.tasks) != 0:
			add_to_set.add(r)

	return add_to_set

def enqueue_steps(ready_set, cookbook):
	new_in_set = []
	remove_from_set = set()
	tasks = [s[1] for s in cookbook.queue]

	# Add steps of tasks not in queue to queue
	for recipe in ready_set:
		if len(recipe.tasks) > 0:
			task = recipe.tasks[0]
			if task not in tasks:
				for step in task.steps:
					cookbook.queue.append((step, task))
		# No tasks for this recipe
		else:
			# Remove recipe from ready_set and add it's dependents 
			remove_from_set.add(recipe)
			if recipe.name == cookbook.main_recipe:
				recipe.done = True
			new_in_set.append(remove_recipe(recipe, cookbook, ready_set))
	
	# Adding new recipes
	for new in new_in_set:
		ready_set |= new
		for recipe in new:
			task = recipe.tasks[0]
			for step in task.steps:
				cookbook.queue.append((step, task))
	ready_set -= remove_from_set

	# return whether a recipe has completed
	return len(remove_from_set) != 0

def remove_step(step, cookbook, ready_set):
	# remove completed step
	task = step[1]
	task.steps.remove(step[0])
	update = False

	# If task completed
	if len(task.steps) == 0:
		recipe = task.recipe
		recipe.tasks.remove(task)

		# Check if redirection worked
		if task.piped or (task.in_file or task.out_file):
			inputt = task.message
			if task.in_file:
				f_in = open(task.in_file)
				inputt += f_in.read()
				f_in.close()
			
			if task.out_file:
				f_out = open(task.out_file)
				output = f_out.read()
				f_out.close()
			else:
				global stdout
				output = stdout
			if inputt != output:
				print("ERROR: output not equal to input")
				sys.exit(4)

		update = True
	
	# remove step from queue
	cookbook.queue.remove(step)
	if update:
		# enqueue next task if recipe completed task
		return enqueue_steps(ready_set, cookbook)
	return False

def analyze_transcript(cookbook, transcript, main_recipe, max_cooks):
	if not main_recipe:
		main_recipe = cookbook.recipes[0].name

	cookbook.main_recipe = main_recipe
	root = get_recipe(cookbook, main_recipe)
	root.done = False
	ready_set = set()
	get_leaves(root, ready_set)
	enqueue_steps(ready_set, cookbook)

	cooks = 0
	pid_to_step = {}
	tscript = io.StringIO(transcript)
	prev_start = None

	for line in tscript:

		print(line.strip())

		info = get_stub_info(line.strip())

		if prev_start and info.start_time - prev_start.start_time > WAIT_TIME:
			print("ERROR: recipes in queue waiting too long (recipes ready/cooking: " +
                              str(len(ready_set)) + ", active cooks: " + str(cooks) + ", max cooks: " +
                              str(max_cooks) + ")")
			return 3

		words = info.words.split(' ')
		if info.is_start:
			# Check if this is a step in the ready queue
			for s in cookbook.queue:
				s_check = s[0].split(' ')
				if s_check[0] in words[0] and ' '.join(words[1:]) == ' '.join(s_check[1:]):
					# Check if exceeding max cooks if not increment cooks
					if not s[1].recipe.in_progress:
						if cooks + 1 > max_cooks:
							print("ERROR: cooks (" + str(cooks+1) +
                                                              ") exceeded max cooks (" + str(max_cooks) + ")")
							return 2
						cooks += 1
						if cooks < max_cooks and cooks < len(ready_set):
							prev_start = info
						else: prev_start = None
						s[1].recipe.in_progress = True

					pid_to_step[info.pid] = s
					break
			else:
				print("ERROR: step '" + str(info.words) + "' should not have started")
				return 1
		else:
			try:
				step = pid_to_step[info.pid]
			except:
				return -2
			#print(ready_set, step)
			if remove_step(step, cookbook, ready_set):
				cooks -= 1
			del pid_to_step[info.pid]
	if root.done:
		print("SUCCESSFUL COMPLETION OF RECIPE: " + main_recipe)
		return 0
	print("ERROR: Student program terminated but main recipe not complete")
	return 1

def compare_output(output, expected_output):
	return output.strip() == expected_output

if __name__ == '__main__':
	parsed = parse_args()

	argv = ('{:s} {:s} {:s} {:s}'.format(parsed.p,\
					'-f ' + parsed.f if parsed.f else '', \
					'-c ' + str(parsed.c) if parsed.c else '', \
					'' if not parsed.m else parsed.m)).split()

	parsed.f = 'rsrc/cookbook.ckb' if not parsed.f else parsed.f
	parsed.c = 1 if not parsed.c else parsed.c

	WAIT_TIME = parsed.w / 1000
	cookbook = parse_cookbook(parsed.f)
	transcript, stdout, returncode = get_student_transcript(argv, len(cookbook.recipes))
	if returncode == -signal.SIGSEGV:
		print("ERROR: student program segfaulted")
		sys.exit(1)
	if parsed.x:
		if compare_output(stdout, parsed.x) == 0:
			print("ERROR: output incorrect, was {} expected {}".format(stdout, parsed.x))
			sys.exit(1)
		else:
			print("SUCCESS: Expected Output produced")
			sys.exit(0)
	if parsed.e:
		if returncode != 0:
			print("SUCCESSFUL HANDLING OF ERROR (student returned non-zero)")
			sys.exit(0)
		else:
			print("ERROR: student returned " + str(returncode) + " expected non-zero")
			sys.exit(1)
	elif returncode != 0:
		print("ERROR: student program did not return 0")
		sys.exit(-1)
		
	sys.exit(analyze_transcript(cookbook, transcript, parsed.m, parsed.c))
