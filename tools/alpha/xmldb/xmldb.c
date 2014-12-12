/* vi: set sw=4 ts=4: */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "dtrace.h"
#include "xmldb.h"
#include "rgdb.h"

struct xml_node g_xml_root = { NULL, NULL, 0, NULL, NULL, NULL };



static struct xml_node * remove_child(struct xml_node * parent, const char * name, int id);

/****************************************************************************/

const char * xmldb_eatwhite(const char * string)
{
	if (string == NULL) return NULL;
	while (*string)
	{
		if (!IS_WHITE(*string)) break;
		string++;
	}
	return string;
}

char * xmldb_reatwhite(char * string)
{
	int i;
	if (string == NULL) return NULL;
	i = strlen(string)-1;
	while (i>=0 && IS_WHITE(string[i])) string[i--] = '\0';
	return string;
}

/****************************************************************************/

/* allocate/init memory for struct xml_node */
static struct xml_node * new_node(const char * name, const char * value, int id)
{
	struct xml_node * node=NULL;
	int nlen;
	char * ptr;

	if (name)
	{
		nlen = strlen(name);
		if (nlen > 0)
		{
			node = (struct xml_node *)malloc(sizeof(struct xml_node)+nlen+1);
			if (node)
			{
				memset(node, 0, sizeof(struct xml_node)+nlen+1);
				ptr = (char *)(&node[1]);
				strcpy(ptr, name);
				node->id = id;
				node->name = ptr;
				if (value) node->value = strdup(value);
			}
		}
	}
	return node;
}

/* destroy/free a xml_node and all the child. */
static void del_node(struct xml_node * node)
{
	struct xml_node * cnode, * nnode;

	dassert(node!=NULL);
	cnode = node->child;
	while (cnode)
	{
		nnode = cnode->next;
		del_node(cnode);
		cnode = nnode;
	}

	if (node->value) free(node->value);
	if (node->notify) free(node->notify);
	free(node);
}

static int read_tag(FILE * fh, char * value, size_t vsize, char * tag, size_t tsize)
{
	size_t v, t;
	int c;
	int state = 0;
	char rawvalue[MAX_VAL_LEN+1];
	char * raw;

	v = t = 0;
	for (;;)
	{
		c = fgetc(fh);
		if (c == EOF) break;
		if (state == 0)
		{	/* looking for '<' */
			if (c == '<')
			{	/* no more value, tag start */
				tag[t++] = (char)c;
				state++;
			}
			else
			{
				rawvalue[v++] = (char)c;
				dassert(v < MAX_VAL_LEN);
				if (v >= MAX_VAL_LEN) break;
			}
		}
		else
		{	/* looking for '>' */
			dassert(t < tsize);
			if (t >= tsize) break;
			if (c == '>')
			{
				tag[t++] = '>';
				if ((t > 4 && strncmp(tag, "<!--", 4)==0) || (t > 5 && strncmp(tag, "<?xml", 5)==0))
				{	/* it's comment, skip this tag */
					t = 0;
					state = 0;
				}
				else
				{
					break;
				}
			}
			else
			{
				tag[t++] = (char)c;
			}
		}
	}
	tag[t] = '\0';
	if (value)
	{	/* Handle special characters.
		 * '&':&amp;   - ampersand,
		 * '<':&lt;    - less then,
		 * '>':&gt;    - greater then,
		 * '"':&quot;  - quotation mark,
		 * ''':&apos;  - apostrophe.		*/
		rawvalue[v] = '\0';
		raw = rawvalue;
		v = 0;
		while (*raw && v < vsize)
		{
			if (*raw == '&')
			{
				if		(strncmp(raw, "&amp;",  5)==0) { value[v++] = '&'; raw += 5; }
				else if	(strncmp(raw, "&lt;",   4)==0) { value[v++] = '<'; raw += 4; }
				else if (strncmp(raw, "&gt;",   4)==0) { value[v++] = '>'; raw += 4; }
				else if (strncmp(raw, "&quot;", 6)==0) { value[v++] = '"'; raw += 6; }
				else if (strncmp(raw, "&apos;", 6)==0) { value[v++] = '\''; raw += 6; }
				else
					value[v++] = *raw++;
			}
			else
			{
				value[v++] = *raw++;
			}
		}
		value[v] = '\0';
	}
	return c;
}

static int get_name_from_tag(const char * tag, char * buff, int size)
{
	int i;
	int j=0;

	if (tag[0] != '<') return 0;
	for (i=1; tag[i] && !IS_WHITE(tag[i]) && tag[i]!='>' && j < size-1; i++) buff[j++] = tag[i];
	buff[j] = '\0';
	return j;
}

static int get_id_from_tag(const char * tag)
{
	char * str;
	char buff[32];
	int i = 0;

	str = strstr(tag, "id=");
	if (str)
	{
		while (*str != '"') str++;
		str++;
		while (*str != '"') buff[i++] = *str++;
		buff[i]='\0';
		return atoi(buff);
	}
	return 0;
}

static struct xml_node * parse_xmltree(FILE * fh, char * tag, int tsize, char * val, int vsize, int depth)
{
	int i, c;
	struct xml_node * node = NULL;
	char name[MAX_TAG_LEN];

	//d_dbg("parse_xmltree(%d): tag[%s], val[%s]\n", depth, tag, xmldb_eatwhite(val));

	/* create new node */
	i = get_name_from_tag(tag, name, sizeof(name));
	if (i <= 0) return NULL;
	//d_dbg("parse_xmltree(%d): new_node(%s, %d)\n", depth, name, get_id_from_tag(tag));
	node = new_node(name, NULL, get_id_from_tag(tag));
	dassert(node);
	if (!node) return NULL;

	/* get the value and next tag. */
	c = read_tag(fh, val, vsize, tag, tsize);
	//d_dbg("parse_xmltree(%d): val[%s], tag[%s]\n", depth, xmldb_eatwhite(val), tag);

	/* set value */
	if (strlen(xmldb_eatwhite(xmldb_reatwhite(val))) > 0)
	{
		node->value = strdup(xmldb_eatwhite(val));
		//d_dbg("parse_xmltree(%d): set value [%s]=[%s]\n", depth, node->name, node->value);
	}

	/* extract tag name */
	i = get_name_from_tag(tag, name, sizeof(name));
	if (i <= 0)
	{
		d_warn("parse_xmltree(%d): no tag available!\n", depth);
		del_node(node);
		return NULL;
	}

	/* Is there a child ? */
	if (name[0] != '/')
	{
		node->child = parse_xmltree(fh, tag, tsize, val, vsize, depth+1);
	}

	//d_dbg("parse_xmltree(%d): val[%s], tag[%s]\n", depth, xmldb_eatwhite(val), tag);
	i = get_name_from_tag(tag, name, sizeof(name));
	if (i <= 0)
	{
		d_warn("parse_xmltree(%d): no tag available!\n", depth);
		del_node(node);
		return NULL;
	}
	if (name[0] != '/')
	{
		d_warn("parse_xmltree(%d): should be an end tag!\n", depth);
		del_node(node);
		return NULL;
	}

	if (strcmp(node->name, &name[1])!=0) return node;

	/* read next */
	c = read_tag(fh, NULL, 0, tag, tsize);
	//d_dbg("parse_xmltree(%d): next tag[%s]\n", depth, tag);
	i = get_name_from_tag(tag, name, sizeof(name));
	if (i > 0 && name[0] != '/') node->next = parse_xmltree(fh, tag, tsize, val, vsize, depth+1);
	return node;
}

/* create XML tree from XML file. */
int xmldb_read_xml(const char * file, unsigned long flags)
{
	FILE * fh;
	char tag[MAX_TAG_LEN], val[MAX_VAL_LEN];
	int c, i;
	struct xml_node * nptr = NULL;
	struct xml_node * tmp;

	d_dbg("xmldb_read_xml(%s)\n", file);

	/* if skip proc flags is set, keep 'proc' node. */
	if (flags & RGDB_SKIP_PROC)
	{
		nptr = remove_child(&g_xml_root, "proc", 0);
	}
	
	/* delete root node */
	if (g_xml_root.child) del_node(g_xml_root.child);
	g_xml_root.child = NULL;

	/* open XML file */
	fh = fopen(file, "r");
	if (fh==NULL) return -1;

	/* get the root node. */
	c = read_tag(fh, NULL, 0, tag, sizeof(tag));
	i = get_name_from_tag(tag, val, sizeof(val));
	d_dbg("i: %d, tag:[%s], val:[%s]\n",i, tag, val);
	if (i > 0 && strcmp(val, o_rootname)==0)
	{
		c = read_tag(fh, NULL, 0, tag, sizeof(tag));
		i = get_name_from_tag(tag, val, sizeof(val));
		d_dbg("tag:[%s], val:[%s]\n", tag, val);
		if (i > 0) g_xml_root.child = parse_xmltree(fh, tag, sizeof(tag), val, sizeof(val), 0);
	}

	if (nptr)
	{
		tmp = remove_child(&g_xml_root, "proc", 0);
		if (tmp) del_node(tmp);
		tmp = g_xml_root.child;
		if (tmp)
		{
			while (tmp->next) tmp = tmp->next;
			tmp->next = nptr;
		}
		else
		{
			g_xml_root.child = nptr;
		}
	}

	return (g_xml_root.child == NULL) ? -1 : 0;
}

/****************************************************************************/
/* Dump nodes to XML format */

/* print start tag, id and value */
static void dump_node_start(FILE * fh, struct xml_node * node, int depth)
{
	int i;
	char * val;

	for (i=0; i<depth; i++) fprintf(fh, "\t");
	fprintf(fh, "<%s", node->name);
	if (node->id > 0) fprintf(fh, " id=\"%d\"", node->id);
	fprintf(fh, ">");
	if (node->value)
	{
		val = node->value;
		while (*val)
		{	/* handle the special characters:
			 * '&' -> '&amp;'  - ampersand,
			 * '<' -> '&lt;'   - less then,
			 * '>' -> '&gt;'   - greater then,
			 * ''' -> '&apos;' - apostrophe,
			 * '"' -> '&quot;' - quotation mark.	*/
			switch (*val)
			{
			case '&':  fprintf(fh, "&amp;");	val++; break;
			case '<':  fprintf(fh, "&lt;");		val++; break;
			case '>':  fprintf(fh, "&gt;");		val++; break;
			case '"':  fprintf(fh, "&quot;");	val++; break;
			case '\'': fprintf(fh, "&apos;");	val++; break;
			default:
				fprintf(fh, "%c", *val++);
				break;
			}
		}
	}
}

/* print end tag */
static void dump_node_end(FILE * fh, struct xml_node * node, int depth)
{
	int i;
	for (i=0; i<depth; i++) fprintf(fh, "\t");
	fprintf(fh, "</%s>\n", node->name);
}

/* dump nodes */
void xmldb_dump_nodes(FILE * fh, struct xml_node * node, int depth, unsigned long flags)
{
	struct xml_node * cnode;

	cnode = node;
	while (cnode)
	{
		/* Check if this node should be ignored. */
		if (depth == 1)
		{
			if (strcmp(cnode->name, "tmp")==0 ||
				strcmp(cnode->name, "runtime")==0)
				goto advance_to_next;

			/* skip 'proc', if flag is set. */
			if ((flags & RGDB_SKIP_PROC) && strcmp(cnode->name, "proc")==0)
				goto advance_to_next;
		}

		/* start dumping */
		dump_node_start(fh, cnode, depth);
		if (cnode->child)
		{
			fprintf(fh, "\n");
			xmldb_dump_nodes(fh, cnode->child, depth+1, flags);
			dump_node_end(fh, cnode, depth);
		}
		else
			dump_node_end(fh, cnode, 0);

		/* advance to next. */
advance_to_next:
		cnode = cnode->next;
	}
}

static const char * get_node(char * buf, size_t size, const char * path)
{
	int i=0;
	if (buf && size > 0)
	{
		for (i=0; path[i] && path[i]!='/' && i < size-1; i++) buf[i] = path[i];
		buf[i] = '\0';
	}
	while (path[i] && path[i]!='/') i++;
	if (path[i]=='/') i++;
	return &path[i];
}

struct xml_node * xmldb_find_sibling(struct xml_node * start, const char * name, int id, int create)
{
	struct xml_node * curr = start;
	struct xml_node * prev = NULL;

	while (curr)
	{
		if (strcasecmp(curr->name, name)==0 && curr->id == id) return curr;
		prev = curr;
		curr = curr->next;
	}
	if (create)
	{
		curr = new_node(name, NULL, id);
		prev->next = curr;
	}
	return curr;
}

static struct xml_node * remove_child(struct xml_node * parent, const char * name, int id)
{
	struct xml_node * curr = parent->child;
	struct xml_node * prev = NULL;

	while (curr)
	{
		if (strcmp(curr->name, name) == 0 && curr->id == id)
		{
			if (parent->child == curr)	parent->child = curr->next;
			else						prev->next = curr->next;

			curr->next = NULL;
			break;
		}
		prev = curr;
		curr = curr->next;
	}
	return curr;
}

/* find node */
struct xml_node * xmldb_find_node(struct xml_node * root, const char * path, int create)
{
	char buff[MAX_TAG_LEN];
	const char * ptr = path;
	char * idptr;
	int id;
	struct xml_node * target = NULL;

#if 0
	d_dbg("xmldb_find_node(%d, path=%s)\n", create, path);
	d_dbg("   root = 0x%x\n", root);
	if (root)
	{
		d_dbg("   name = %s, id=%d\n", root->name, root->id);
	}
#endif
	
	/* start from root node ? */
	if (*ptr == '/')
	{
		root = &g_xml_root;
		ptr++;
	}
	if (!root) root = &g_xml_root;

	while (*ptr)
	{
		/* extract a node name and id. */
		ptr = get_node(buff, sizeof(buff), ptr);
		//d_dbg("get_node(%s), [%s]\n", buff, ptr);
		if (strcmp(buff, ".")==0)
		{
			target = root;
			continue;
		}
		idptr = strchr(buff, ':');
		if (idptr)
		{
			*idptr++ = '\0';
			id = atoi(idptr);
		}
		else
		{
			id = 0;
		}

		if (root->child)
		{
			target = xmldb_find_sibling(root->child, buff, id, create);
		}
		else if (create)
		{
			target = new_node(buff, NULL, id);
			root->child = target;
		}

		if (!target) break;
		root = target;
	}
	return target;
}

/* send notify message */
extern void send_notify(const char * message);

/* set node */
struct xml_node * xmldb_set_value(struct xml_node * root, const char * path, const char * value)
{
	struct xml_node * node;
	int dirty = 0;

	node = xmldb_find_node(root, path, 1);
	if (node)
	{
		if (node->value)
		{
			if (strcmp(node->value, value)!=0)
			{
				free(node->value);
				node->value = NULL;
				if (value[0]) node->value = strdup(value);
				dirty++;
			}
		}
		else
		{
			if (value[0])
			{
				node->value = strdup(value);
				dirty++;
			}
		}

		if (dirty && node->notify) send_notify(node->notify);
	}
	return node;
}

/* set notify node */
struct xml_node * xmldb_set_notify(struct xml_node * root, const char * path, const char * message)
{
	struct xml_node * node;

	node = xmldb_find_node(root, path, 1);
	if (node)
	{
		if (node->notify)
		{
			free(node->notify);
			node->notify = NULL;
		}
		if (message[0]) node->notify = strdup(message);
	}
	return node;
}

/* get node */
const char * xmldb_get_value(struct xml_node * root, const char * path)
{
	struct xml_node * node;
	const char * ptr = NULL;

	//d_dbg("xmldb_get_value(root=0x%x, path=%s)\n",root, path);

	node = xmldb_find_node(root, path, 0);
	if (node) ptr = node->value;
	return ptr ? ptr : "";
}

/* del node */
int xmldb_del_node(struct xml_node * root, const char * path)
{
	char * buff;
	char * ppath;
	char * lnode;
	char * idptr;
	int id;
	int ret;
	struct xml_node * pnode, * node;

	/* extract the leaf node */
	ppath = buff = strdup(path);
	lnode = strrchr(ppath, '/');
	if (lnode)
	{
		if (lnode == ppath) ppath = NULL;
		*lnode++ = '\0';
	}
	else
	{
		lnode = ppath; ppath = NULL;
	}

	/* extract id from node name */
	idptr = strchr(lnode, ':');
	if (idptr)
	{
		*idptr++ = '\0';
		id = atoi(idptr);
	}
	else
	{
		id = 0;
	}

	d_dbg("xmldb_del_node: parent:%s, lnode:%s, id:%d\n", ppath, lnode, id);

	/* find the parent node */
	if (ppath) pnode = xmldb_find_node(root, ppath, 0);
	else if (root) pnode = root;
	else pnode = &g_xml_root;

	do
	{
		ret = -1;
		if (!pnode) break;
	
		d_dbg("xmldb_del_node: find child node [%s], id=%d\n", lnode, id);
		node = remove_child(pnode, lnode, id);
		if (!node) break;

		d_dbg("xmldb_del_node: found, delete!\n");
		del_node(node);
		ret = 0;

		if (id == 0) break;

		do
		{
			d_dbg("xmldb_del_node: find %s id = %d\n", lnode, id+1);
			node = xmldb_find_sibling(pnode->child, lnode, id+1, 0);
			if (node)
			{
				d_dbg("xmldb_del_node: found %s id=%d, change id to %d\n", lnode, node->id, id);
				node->id = id;
			}
			id++;
		} while (node);
	} while (0);

	free(buff);
	return ret;
}

/****************************************************************************/
/* for debug */
#if 0
int main(int argc, char * argv[])
{
	struct xml_node * node;

	//xmldb_read_xml("test.xml");

	xmldb_set_value(NULL, "/wan/test/test1/test2/test3/test4/node", "test node");
	xmldb_set_value(NULL, "/wan/test/node:1", "test node 1");
	xmldb_set_value(NULL, "/wan/test/node:2", "test node 2");
	xmldb_set_value(NULL, "/wan/test/node:3", "test node 3");
	xmldb_set_value(NULL, "/wan/test/node:4", "test node 4");
	xmldb_set_value(NULL, "/wan/test/node:5", "test node 5");

	node = xmldb_find_node(NULL, "/wan/test/test1", 0);
	if (node)
	{
		node = xmldb_find_node(node, "node2/node3/node4", 1);
		node->value = strdup("node 4");
	}

	printf("/wan/test/test1/test2/test3/test4/node = [%s]\n", xmldb_get_value(NULL, "/wan/test/test1/test2/test3/test4/node"));
	printf("/wan/test/node:2 = [%s]\n", xmldb_get_value(NULL, "/wan/test/node:2"));
	xmldb_del_node(NULL, "/wan/test/node:2");
	printf("/wan/test/node:2 = [%s]\n", xmldb_get_value(NULL, "/wan/test/node:2"));

	xmldb_dump_nodes(stdout, g_xml_root.child, 0);

	del_node(g_xml_root.child);
	return 0;
}
#endif

/****************************************************************************/

