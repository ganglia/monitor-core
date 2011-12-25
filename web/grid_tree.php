<?php
$tpl = new Dwoo_Template_File( template("grid_tree.tpl") );
$data = new Dwoo_Data();

$data->assign("self", "$self");

# Not as complicated as before. No depth-first-search, and
# we only show our immediate children.

# Publish past grids in our stack.
$ancestors = $gridstack;
# Take ourself off the end of the stack.
array_pop($ancestors);

if (count($ancestors)) {
   $data->assign("parentgrid", 1);
   $parentgridtable = "";

   $parentgridstack = array();
   foreach ($ancestors as $g) {
      list($name, $link) = explode("@", $g);
      $parentgridstack[] = $g;
      $parentgridstack_url = rawurlencode(join(">", $parentgridstack));
      $parentgridtable .= "<tr><td align=center class=grid>".
         "<a href=\"$link?t=yes&amp;gw=back&amp;gs=$parentgridstack_url\">$name</a></td></tr>\n";
   }

   $data->assign("parents", $parentgridtable);
}

$gridtable="";

# Publish our children.
if ($n = count($grid))
   {
      $data->assign("n", $n);
      foreach ($grid as $source => $attrs)
         {
            if ($source == $self) continue;

            if (isset($grid[$source]['GRID']) and $grid[$source]['GRID'])
               {
                  # This child is a grid.
                  $url = $grid[$source]['AUTHORITY'] . "?t=yes&amp;gw=fwd&amp;gs=$gridstack_url";
                  $gridtable .= "<td class=grid><a href=\"$url\">$source</a></td>";
               }
            else
               {
                  # A cluster.
                  $url = "./?c=". rawurlencode($source) ."&amp;$get_metric_string";
                  $gridtable .= "<td><a href=\"$url\">$source</a></td>";
               }
         }
      $gridtable .= "</tr></table>";
   }

$data->assign("children", $gridtable);

$dwoo->output($tpl, $data);

?>
