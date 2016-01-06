"""
A script to extract description from my Code into MD format, for documentational purpose

Source format:
/**STRUCT+********************************************************************/
/* Structure: HM_NOTIFICATION_CB											 */
/*                                                                           */
/* Name:      hm_notification_cb			 								 */
/*                                                                           */
/* Textname:  Notification Control Block                                     */
/*                                                                           */
/* Description: A Notification Event which needs to be processed. This would */
/* be used differently in different perspectives. The same notification can  */
/* be used to broadcast an update to the peers and a notification to the 	 */
/* subscribed nodes.														 */
/*                                                                           */
/*****************************************************************************/
typedef struct hm_notification_cb
{
	/***************************************************************************/
	/* Node in list															   */
	/***************************************************************************/
	HM_LQE node;

	/***************************************************************************/
	/* Notification Type													   */
	/***************************************************************************/
	uint16_t notification_type;

	/***************************************************************************/
	/* Affected CB															   */
	/***************************************************************************/
	void *node_cb;

	/***************************************************************************/
	/* Notification State: User specific.									   */
	/***************************************************************************/
	void *custom_data;

} HM_NOTIFICATION_CB ;
/**STRUCT-********************************************************************/


Target Format:
1. <Structure Name>(HM_NOTIFICATION_CB)
	
	**Purpose**: <Description>

	**Structure**: 
		```
		typedef...
		```

	**Information**:

		|	Field			|			description     |
		|-------------------|---------------------------|
		|node 				|			Node in list	|
		|notification_type	|			Notification typedef|
		|node_cb			|			Affected CB 	|
		| custom_data		|		Notification State: User specific. |


"""
import sys
import os
import re

class Error(Exception):
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return str(self.msg)

class Structure(object):

	def __init__(self):
		self.name = None #Name of structure
		self.purpose = None # Description field
		self.structure = None #Code, sans its comments
		self.information = {} #Dictionary

## ******************** REGEXES ******************** ##
struct_start_re = re.compile(r'^.*\*STRUCT\+\**')

struct_name = re.compile(r'^.*Structure:\s?(?P<name>\w)\s?\*/')
struct_purpose = re.compile(r'^.*Description:\s?(?P<descr>\w)\s?\*/')
struct_purpose_line = re.compile(r'^/\*\s?(?P<line>.*)\*/')
struct_comment_boundary = re.compile(r'^/\*(\*)+\*/')

struct_start = re.compile(r'^\s?(?P<line>typedef\s+struct\s+\w+.*)')
struct_var_declaration = re.compile(r'^\s*(?P<type>\w)\s+(?P<var_name>\w)\s?;.*')
struct_end = re.compile(r'^\s?(}\s+(?P<name>\w)\s?;)')

struct_end_re = re.compile(r'\*STRUCT-')
## ******************** MAIN CODE ******************** ##

if len(sys.argv) is not 2:
	raise Error("No file specified.")

if not sys.argv[1].endswith('.h'):
	raise Error("Specified file is not a header file.")

fd = open(sys.argv[1], 'r')
md_target = open('conv.md', 'w')

line_num = 0
#We have the file opened. Now read it line by line and look for beginning of structures:
looking_for_struct_start = True
finding_purpose = False
finding_info = False
current_info = ''
while 1:
	line = fd.readline()
	line_num +=1
	if not line:
		print "End of file reached."
		exit()
	#Are we looking for a new structure to add, or are we making an old one
	if looking_for_struct_start:
		match= struct_start_re.match(line)
		if match is not None:
			#Found structure start
			print(line)
			struct = Structure()
			looking_for_struct_start = False
	else:
		if struct.name is None:
			match = struct_name.match(line)
			if match is not None:
				struct.name = match.group('name')
				continue
		if struct.purpose is None or finding_purpose is True:
			if struct.purpose is None:
				match = struct_purpose.match(line)
				if match is not None:
					struct.purpose = match.group('descr')
					finding_purpose = True
					continue
			if finding_purpose:
				match = struct_comment_boundary.match(line)
				if match is not None:
					match = struct_purpose_line.match(line)
					if match is not None:
						struct.purpose += match.group('line')
						continue
				else:
					#We've found our purpose
					finding_purpose = False
					continue
		if not struct.information: #Empty dictionary
			#start looking for information
			match = struct_start.match(line)
			if match is not None:
				finding_info = True
				struct.structure = match.group('line')
				continue

		if finding_info is True:
			#We're parsing the code segment now. 
			#Search for comments before declarations
			match = struct_comment_boundary.match(line)
			if match is not None:
				parsing_info = True
				while parsing_info is True:
					line = fd.readline()
					line_num +=1
					match = struct_comment_boundary.match(line)
					if match is not None:
						if len(current_info) is 0:
							raise Error('Empty comment column at line {line}'.format(line=line_num))
						else:
							parsing_info = False
							continue
					match =  struct_purpose_line.match(line)
					if match is not None:
						current_info += match.group('line')
						continue

			#Search for declarations. Add them to code
			match = struct_var_declaration.match(line)
			if match is not None:
				struct.structure += '\n'
				struct.structure += line
				struct.information[match.group('var_name')] = current_info
				continue

			#Search for end of structure
			match = struct_end.match(line)
			if match is not None:
				struct.structure += '\n'
				struct.structure +=line
				finding_info = False
				current_info = ''
				continue

			#If it is none of these, just add it to the structure
			struct.structure += '\n'
			struct.structure +=line
			continue

		match = struct_end_re.match(line)
		
		if match is not None:
			looking_for_struct_start = True
			#Write the structure as markdown
			'''
			1. <Structure Name>(HM_NOTIFICATION_CB)
	
				**Purpose**: <Description>

				**Structure**: 
					```
					typedef...
					```

				**Information**:

					|	Field			|			description     |
					|-------------------|---------------------------|
					|node 				|			Node in list	|
					|notification_type	|			Notification typedef|
					|node_cb			|			Affected CB 	|
					| custom_data		|		Notification State: User specific. |
			'''
			md_target.write('1. {name}'.format(name=struct.name))
			print('1. {name}'.format(name=struct.name))
			md_target.write('    **Purpose**: {descr}'.format(name=struct.purpose))
			print('    **Purpose**: {descr}'.format(name=struct.purpose))
			md_target.write('\n')
			print('\n')
			md_target.write('    **Structure**:')
			print('    **Structure**:')
			md_target.write('        ```')
			print('        ```')
			md_target.write('        {structure}'.format(structure = struct.structure))
			print('        {structure}'.format(structure = struct.structure))
			md_target.write('        ```')
			print('        ```')
			md_target.write('    **Information**:')
			print('    **Information**:')
			md_target.write('        | Field |	description |')
			print('        | Field |	description |')
			md_target.write('        |-------|--------------|')
			print('        |-------|--------------|')
			for key,value in struct.information:
				md_target.write('        | {key} |	{value} |'.format(key=key, value=value))
				print('        | {key} |	{value} |'.format(key=key, value=value))

#Out of the loop now
fd.close()
md_target.close()