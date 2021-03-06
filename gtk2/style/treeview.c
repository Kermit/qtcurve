/*****************************************************************************
 *   Copyright 2003 - 2010 Craig Drummond <craig.p.drummond@gmail.com>       *
 *   Copyright 2013 - 2014 Yichao Yu <yyc1992@gmail.com>                     *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU Lesser General Public License as          *
 *   published by the Free Software Foundation; either version 2.1 of the    *
 *   License, or (at your option) version 3, or any later version accepted   *
 *   by the membership of KDE e.V. (or its successor approved by the         *
 *   membership of KDE e.V.), which shall act as a proxy defined in          *
 *   Section 6 of version 3 of the license.                                  *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 *   Lesser General Public License for more details.                         *
 *                                                                           *
 *   You should have received a copy of the GNU Lesser General Public        *
 *   License along with this library. If not,                                *
 *   see <http://www.gnu.org/licenses/>.                                     *
 *****************************************************************************/

#include <qtcurve-utils/gtkprops.h>
#include <qtcurve-cairo/utils.h>

typedef struct {
    GtkTreePath       *path;
    GtkTreeViewColumn *column;
    gboolean          fullWidth;
} QtCTreeView;

static GHashTable *qtcTreeViewTable=NULL;

static QtCTreeView * qtcTreeViewLookupHash(void *hash, gboolean create)
{
    QtCTreeView *rv=NULL;

    if(!qtcTreeViewTable)
        qtcTreeViewTable=g_hash_table_new(g_direct_hash, g_direct_equal);

    rv=(QtCTreeView *)g_hash_table_lookup(qtcTreeViewTable, hash);

    if(!rv && create)
    {
        rv = qtcNew(QtCTreeView);
        rv->path=NULL;
        rv->column=NULL;
        rv->fullWidth=false;
        g_hash_table_insert(qtcTreeViewTable, hash, rv);
        rv=g_hash_table_lookup(qtcTreeViewTable, hash);
    }

    return rv;
}

static void qtcTreeViewRemoveFromHash(void *hash)
{
    if(qtcTreeViewTable)
    {
        QtCTreeView *tv=qtcTreeViewLookupHash(hash, false);
        if(tv)
        {
            if(tv->path)
                gtk_tree_path_free(tv->path);
            g_hash_table_remove(qtcTreeViewTable, hash);
        }
    }
}

void qtcTreeViewGetCell(GtkTreeView *treeView, GtkTreePath **path,
                        GtkTreeViewColumn **column, int x, int y,
                        int width, int height)
{
    const GdkPoint points[4] = {{x + 1, y + 1}, {x + 1, y + height - 1},
                                {x + width - 1, y + 1},
                                {x + width, y + height - 1}};
    for (int pos = 0;pos < 4 && !(*path);pos++) {
        gtk_tree_view_get_path_at_pos(treeView, points[pos].x,
                                      points[pos].y, path, column, 0L, 0L);
    }
}

static void
qtcTreeViewCleanup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && qtcWidgetProps(props)->treeViewHacked) {
        qtcTreeViewRemoveFromHash(widget);
        qtcDisconnectFromProp(props, treeViewDestroy);
        qtcDisconnectFromProp(props, treeViewUnrealize);
        qtcDisconnectFromProp(props, treeViewStyleSet);
        qtcDisconnectFromProp(props, treeViewMotion);
        qtcDisconnectFromProp(props, treeViewLeave);
        qtcWidgetProps(props)->treeViewHacked = false;
    }
}

static gboolean
qtcTreeViewStyleSet(GtkWidget *widget, GtkStyle *prev_style, void *data)
{
    QTC_UNUSED(prev_style);
    QTC_UNUSED(data);
    qtcTreeViewCleanup(widget);
    return false;
}

static gboolean
qtcTreeViewDestroy(GtkWidget *widget, GdkEvent *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    qtcTreeViewCleanup(widget);
    return false;
}

static gboolean qtcTreeViewSamePath(GtkTreePath *a, GtkTreePath *b)
{
    return a ? (b && !gtk_tree_path_compare(a, b)) : !b;
}

static void
qtcTreeViewUpdatePosition(GtkWidget *widget, int x, int y)
{
    if (GTK_IS_TREE_VIEW(widget)) {
        QtCTreeView *tv = qtcTreeViewLookupHash(widget, false);
        if (tv) {
            GtkTreeView *treeView = GTK_TREE_VIEW(widget);
            GtkTreePath *path = NULL;
            GtkTreeViewColumn *column = NULL;

            gtk_tree_view_get_path_at_pos(treeView, x, y, &path,
                                          &column, 0L, 0L);

            if (!qtcTreeViewSamePath(tv->path, path)) {
                // prepare update area
                // get old rectangle
                QtcRect oldRect = {0, 0, -1, -1 };
                QtcRect newRect = {0, 0, -1, -1 };
                QtcRect updateRect;
                QtcRect alloc = qtcWidgetGetAllocation(widget);

                if (tv->path && tv->column) {
                    gtk_tree_view_get_background_area(
                        treeView, tv->path, tv->column,
                        (GdkRectangle*)&oldRect);
                }
                if (tv->fullWidth) {
                    oldRect.x = 0;
                    oldRect.width = alloc.width;
                }

                // get new rectangle and update position
                if (path && column) {
                    gtk_tree_view_get_background_area(
                        treeView, path, column, (GdkRectangle*)&newRect);
                }
                if (path && column && tv->fullWidth) {
                    newRect.x = 0;
                    newRect.width = alloc.width;
                }

                // take the union of both rectangles
                if (oldRect.width > 0 && oldRect.height > 0) {
                    if (newRect.width > 0 && newRect.height > 0) {
                        qtcRectUnion(&oldRect, &newRect, &updateRect);
                    } else {
                        updateRect = oldRect;
                    }
                } else {
                    updateRect = newRect;
                }

                // store new cell info
                if (tv->path)
                    gtk_tree_path_free(tv->path);
                tv->path = path ? gtk_tree_path_copy(path) : NULL;
                tv->column = column;

                // convert to widget coordinates and schedule redraw
                gtk_tree_view_convert_bin_window_to_widget_coords(
                    treeView, updateRect.x, updateRect.y,
                    &updateRect.x, &updateRect.y);
                gtk_widget_queue_draw_area(
                    widget, updateRect.x, updateRect.y,
                    updateRect.width, updateRect.height);
            }

            if (path) {
                gtk_tree_path_free(path);
            }
        }
    }
}

gboolean qtcTreeViewIsCellHovered(GtkWidget *widget, GtkTreePath *path, GtkTreeViewColumn *column)
{
    QtCTreeView *tv=qtcTreeViewLookupHash(widget, false);
    return tv && (tv->fullWidth || tv->column==column) && qtcTreeViewSamePath(path, tv->path);
}

static gboolean
qtcTreeViewMotion(GtkWidget *widget, GdkEventMotion *event, void *data)
{
    QTC_UNUSED(data);
    if (event && event->window && GTK_IS_TREE_VIEW(widget) &&
        gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget)) == event->window) {
        qtcTreeViewUpdatePosition(widget, event->x, event->y);
    }
    return false;
}

static gboolean
qtcTreeViewLeave(GtkWidget *widget, GdkEventMotion *event, void *data)
{
    QTC_UNUSED(event);
    QTC_UNUSED(data);
    if (GTK_IS_TREE_VIEW(widget)) {
        QtCTreeView *tv = qtcTreeViewLookupHash(widget, false);
        if (tv) {
            GtkTreeView *treeView = GTK_TREE_VIEW(widget);
            QtcRect rect = {0, 0, -1, -1 };
            QtcRect alloc = qtcWidgetGetAllocation(widget);

            if (tv->path && tv->column) {
                gtk_tree_view_get_background_area(
                    treeView, tv->path, tv->column, (GdkRectangle*)&rect);
            }
            if (tv->fullWidth) {
                rect.x = 0;
                rect.width = alloc.width;
            }
            if (tv->path) {
                gtk_tree_path_free(tv->path);
            }
            tv->path = NULL;
            tv->column = NULL;

            gtk_tree_view_convert_bin_window_to_widget_coords(
                treeView, rect.x, rect.y, &rect.x, &rect.y);
            gtk_widget_queue_draw_area(
                widget, rect.x, rect.y, rect.width, rect.height);
        }
    }
    return false;
}

void
qtcTreeViewSetup(GtkWidget *widget)
{
    QTC_DEF_WIDGET_PROPS(props, widget);
    if (widget && !qtcWidgetProps(props)->treeViewHacked) {
        QtCTreeView *tv = qtcTreeViewLookupHash(widget, true);
        GtkTreeView *treeView = GTK_TREE_VIEW(widget);
        GtkWidget *parent = gtk_widget_get_parent(widget);

        if (tv) {
            qtcWidgetProps(props)->treeViewHacked = true;
            int x, y;
#if GTK_CHECK_VERSION(2, 90, 0) /* Gtk3:TODO !!! */
            tv->fullWidth = true;
#else
            gtk_widget_style_get(widget, "row_ending_details",
                                 &tv->fullWidth, NULL);
#endif
            gdk_window_get_pointer(gtk_widget_get_window(widget), &x, &y, 0L);
            gtk_tree_view_convert_widget_to_bin_window_coords(treeView, x, y,
                                                              &x, &y);
            qtcTreeViewUpdatePosition(widget, x, y);
            qtcConnectToProp(props, treeViewDestroy, "destroy-event",
                             qtcTreeViewDestroy, NULL);
            qtcConnectToProp(props, treeViewUnrealize, "unrealize",
                             qtcTreeViewDestroy, NULL);
            qtcConnectToProp(props, treeViewStyleSet, "style-set",
                             qtcTreeViewStyleSet, NULL);
            qtcConnectToProp(props, treeViewMotion, "motion-notify-event",
                             qtcTreeViewMotion, NULL);
            qtcConnectToProp(props, treeViewLeave, "leave-notify-event",
                             qtcTreeViewLeave, NULL);
        }

        if (!gtk_tree_view_get_show_expanders(treeView))
            gtk_tree_view_set_show_expanders(treeView, true);
        if (gtk_tree_view_get_enable_tree_lines(treeView))
            gtk_tree_view_set_enable_tree_lines(treeView, false);

        if (GTK_IS_SCROLLED_WINDOW(parent) &&
            gtk_scrolled_window_get_shadow_type(GTK_SCROLLED_WINDOW(parent)) !=
            GTK_SHADOW_IN) {
            gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(parent),
                                                GTK_SHADOW_IN);
        }
    }
}

gboolean
qtcTreeViewCellIsLeftOfExpanderColumn(GtkTreeView *treeView,
                                      GtkTreeViewColumn *column)
{
    // check expander column
    GtkTreeViewColumn *expanderColumn =
        gtk_tree_view_get_expander_column(treeView);

    if (!expanderColumn || column == expanderColumn) {
        return false;
    } else {
        bool found = false;
        bool isLeft = false;
        // get all columns
        GList *columns = gtk_tree_view_get_columns(treeView);
        for (GList *child = columns;child;child = g_list_next(child)) {
            if (!GTK_IS_TREE_VIEW_COLUMN(child->data)) {
                continue;
            }
            GtkTreeViewColumn *childCol = GTK_TREE_VIEW_COLUMN(child->data);
            if (childCol == expanderColumn) {
                if (found) {
                    isLeft = true;
                }
            } else if (found) {
                break;
            } else if (column == childCol) {
                found = true;
            }
        }
        if (columns) {
            g_list_free(columns);
        }
        return isLeft;
    }
}
